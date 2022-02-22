//
// Created by swift on 2021/5/12.
//

#include "jit_runtime.h"
#include <ir/interpreter.h>

#ifdef __aarch64__
#include <backend/trampolines.h>
#include <backend/arm64/block_linker.h>
#endif

#include "block_translator.h"

constexpr auto max_code_cache = 1024 * 1024 * 256;
constexpr auto jit_hotness = 0x20;

namespace Svm {

    JitRuntime::JitRuntime(UserConfigs &configs) : configs(configs) {
        dispatcher = MakeUnique<Dispatcher>();
        cache_pool = MakeUnique<CachePool>(max_code_cache);
        jit_thread = MakeUnique<JitThread>(configs.jit_threads);
        if (!configs.use_offset_pt) {
            page_table = MakeUnique<FlatPageTable>(configs.address_width, configs.page_bits);
            page_table->Initialize();
        }
        if (Guest64Bit()) {
            block_cache_64 = MakeUnique<BlockCache64>(configs.static_code);
        } else {
            block_cache_32 = MakeUnique<BlockCache32>(configs.static_code);
        }
        switch (configs.guest_arch) {
            case CpuArch::X64:
                run_code_trampoline = __svm_code_run_entry_x64;
                break;
            default:
                UNREACHABLE();
        }
    }

    void JitRuntime::RegisterCore(VCpu *cpu) {
        LockGuard guard(lock);
        cores.emplace(cpu);
        cpu->Context()->help.page_table = GetPageTable().PageTablePtr();
        cpu->Context()->help.context_ptr = cpu;
        cpu->Context()->help.dispatcher_table = GetDispatcher().Data();
        cpu->Context()->help.cache_miss_trampoline = CodeCacheTrampoline();
        cpu->Context()->help.interrupt_trampoline = InterruptTrampoline();
        cpu->Context()->help.call_host_trampoline = CallHostTrampoline();
    }

    void JitRuntime::UnRegisterCore(VCpu *cpu) {
        LockGuard guard(lock);
        cores.erase(cpu);
    }

    void JitRuntime::Run(VCpu *cpu) {
        ASSERT(cpu);
        ASSERT(!cpu->HasInterrupt());
        while (!cpu->HasInterrupt()) {
            auto cache = GetBlockCache(cpu->GetPC());
            if (cache.block->Compiled()) {
                RunJitCache(cpu->Context(), FindAndJit(cache.base));
            } else {
                InterpreterIRBlock(cache, cpu);
            }
        }
    }

    CacheModule *JitRuntime::MapModule(VAddr va, u32 size) {
        LockGuard guard(lock);
        auto itr = mapped_modules.lower_bound(va);
        if (itr != mapped_modules.end() && itr->second->Overlap(va)) {
            return itr->second.get();
        }
        auto res = mapped_modules.emplace(va, std::make_unique<CacheModule>(current_module_id.fetch_add(1), va, size));
        return res.first->second.get();
    }

    CacheModule *JitRuntime::QueryModule(VAddr va) {
        LockGuard guard(lock);
        auto itr = mapped_modules.lower_bound(va);
        if (itr == mapped_modules.end()) {
            return {};
        }
        auto module = itr->second.get();
        if (!module->Overlap(va) && !module->Full()) {
            return {};
        }
        return module;
    }

    CacheModule *JitRuntime::GetModule(u16 id) {
        LockGuard guard(lock);
        auto itr = mapped_modules.find(id);
        if (itr == mapped_modules.end()) {
            return {};
        }
        return itr->second.get();
    }

    void JitRuntime::UnMapModule(VAddr va) {
        LockGuard guard(lock);
        mapped_modules.erase(va);
    }

    void *JitRuntime::FindAndJit(VAddr va, bool fill_dispatcher) {
        if (auto entry = GetDispatcher().Get(va)) {
            return reinterpret_cast<void *>(entry);
        }
        auto cache = GetBlockCache(va);
        auto block = cache.block;
        if (!block->Compiled() && !block->GeneratedIR()) {
            EnsureIRBlock(cache);
        }
        auto res = GetBlockCodeCache(block);
        if (fill_dispatcher) {
            dispatcher->Put(va, res);
        }
        return reinterpret_cast<void *>(res);
    }

    BlockCacheRef JitRuntime::GetBlockCache(VAddr addr) {
        BasicBlock *cache;
        if (block_cache_64) {
            cache = block_cache_64->Emplace(addr);
        } else {
            cache = block_cache_32->Emplace(addr);
        }
        EnsureCache(addr, cache);
        return {addr, cache};
    }

    SharedPtr<IR::IRBlock> JitRuntime::NewIRBlock(VAddr base) {
        return ir_blocks_lru.New(base, &ir_instr_heap);
    }

    PAddr JitRuntime::GetBlockCodeCache(BasicBlock *cache) {
        if (cache->BoundModule()) {
            auto &module_info = cache->GetModule();
            auto &module = mapped_modules[cache->GetModule().module_id];
            return reinterpret_cast<VAddr>(module->GetBoundBuffer(module_info.block_id).stub_data);
        } else {
            return dispatcher->GetEntry(cache->GetDispatchIndex()).value;
        }
    }

    void JitRuntime::EnsureCache(VAddr base, BasicBlock *cache) {
        if (cache->Registered()) {
            return;
        }

        SpinScope guard(cache->Lock());
        if (cache->Registered()) {
            return;
        }
        if (Static()) {
            cache->RegisteredCache();
            return;
        }
        auto module = QueryModule(base);
        if (module) {
            auto id = module->AllocId();
            cache->BindModule(module->GetModuleId(), id);
        } else {
            auto offset = dispatcher->Put(base, reinterpret_cast<VAddr>(ReturnHostTrampoline()));
            cache->BindDispatchTable(offset);
        }
    }

    SharedPtr<IR::IRBlock> JitRuntime::EnsureIRBlock(const BlockCacheRef &cache, bool recursive) {
        auto ir = cache.block->GetIR();
        if (ir) {
            ir_blocks_lru.Notify(ir.get());
            return ir;
        }

        SpinScope guard(cache.block->Lock());
        ir = cache.block->GetIR();
        if (ir) {
            ir_blocks_lru.Notify(ir.get());
            return ir;
        }
        ir = GenerateBlock(this, cache.base, configs.guest_arch);
        if (ir) {
            cache.block->SetIR(ir);
            cache.block->SetSize(ir->BlockSize());
            if (Guest64Bit()) {
                GetCache64().Flush(cache.base, cache.block);
            } else {
                GetCache32().Flush(cache.base, cache.block);
            }
            if (recursive) {
                RecursiveIRBlock(ir);
            }
        }
        return ir;
    }

    void JitRuntime::EnsureCompiled(const BlockCacheRef &cache) {
        if (cache.block->Compiled()) return;
        auto ir_block = EnsureIRBlock(cache);

        SpinScope guard(cache.block->Lock());
        if (!cache.block->Compiled()) {
            if (TranslateBlock(this, ir_block.get())) {
                cache.block->SetCompiled();
                // Delete ir block
                ir_blocks_lru.Delete(ir_block.get());
                // link runtime block
                LockGuard link_guard(lock);
                if (auto itr = pending_link_points.find(cache.base); itr != pending_link_points.end()) {
                    for (auto link_point : itr->second) {
                        LinkBlock(link_point, GetBlockCodeCache(cache.block), cache_pool->RwBuffer(reinterpret_cast<void *>(link_point)), true);
                    }
                    pending_link_points.erase(itr);
                }
            }
        }
    }

    void JitRuntime::PushBlock(VAddr block_start, bool recursive) {
        jit_thread->Push([this, block_start, recursive] {
            auto cache = GetBlockCache(block_start);
            if (cache.block->GeneratedIR()) return;
            auto cache_copy = cache;
            EnsureIRBlock(cache_copy, recursive);
        });
    }

    void JitRuntime::QueueBlockCompile(const BlockCacheRef &cache) {
        if (cache.block->Compiled()) return;
        jit_thread->Push([this, cache] {
            EnsureCompiled(cache);
        });
    }

    void JitRuntime::InterpreterIRBlock(const BlockCacheRef &cache, VirtualCore *core) {
        auto ir_block = EnsureIRBlock(cache);
        ASSERT(ir_block);

        {
            SharedLock<SharedMutex> guard(ir_block->Lock());
            // prepare interp stack
            auto reg_size = ir_block->Sequence().size();
            void *interp_stack{};
            if (reg_size < 0x1000) {
                interp_stack = alloca(reg_size * sizeof(IR::IRReg));
            }

            IR::Interpreter interpreter{this, ir_block.get(), core, interp_stack};
            interpreter.Run();
        }

        if (cache.block->IncreaseHotness() == jit_hotness) {
            QueueBlockCompile(cache);
        }
    }

    void JitRuntime::RunJitCache(CPUContext *context, void *cache) {
//        context->Helper().code_cache = cache;
        context->Helper().halt_flag = false;
        run_code_trampoline(context);
    }

    void JitRuntime::RecursiveIRBlock(const SharedPtr<IR::IRBlock>& ir_block) {
        auto next_blocks = ir_block->NextBlocksAddress(true);
        for (auto block : next_blocks) {
            PushBlock(block, false);
        }
    }

    void* JitRuntime::CodeCacheTrampoline() {
        return reinterpret_cast<void*>(__svm_cache_miss_trampoline);
    }

    void *JitRuntime::CallHostTrampoline() {
        return reinterpret_cast<void *>(__svm_call_host_trampoline);
    }

    void *JitRuntime::InterruptTrampoline() {
        if (GuestArch() == CpuArch::X64) {
            return reinterpret_cast<void *>(__svm_interrupt_trampoline_x64);
        } else {
            return nullptr;
        }
    }

    void *JitRuntime::ReturnHostTrampoline() {
        return reinterpret_cast<void *>(__svm_return_to_host);
    }

    void JitRuntime::PutCodeCache(VAddr pc, PAddr cache) {
        dispatcher->Put(pc, cache);
    }

    PAddr JitRuntime::GetCodeCache(VAddr pc) {
        return GetBlockCodeCache(GetBlockCache(pc).block);
    }

    PAddr JitRuntime::FlushCodeCache(const u8 *buffer, size_t size) {
        abort();
        return 0;
    }

    void JitRuntime::PushLinkPoint(VAddr target, PAddr link_point) {
        LockGuard guard(lock);
        if (auto cache = GetCodeCache(target); cache != 0) {
            auto rw_ptr = cache_pool->RwBuffer(reinterpret_cast<u8 *>(link_point));
            LinkBlock(link_point, cache, rw_ptr);
            return;
        }
        pending_link_points[target].emplace_back(link_point);
    }

    bool JitRuntime::Static() {
        return configs.static_code;
    }

}
