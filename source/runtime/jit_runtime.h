//
// Created by swift on 2021/5/12.
//

#pragma once

#include <base/lru_container.h>
#include <cache/dispatcher_table.h>
#include <memory/memory_interface.h>
#include <memory/page_table.h>
#include <cache/cache_module.h>
#include <cache/cache_pool.h>
#include <include/configs.h>
#include <ir/block.h>
#include "jit_thread.h"

#ifdef __aarch64__
#include <include/svm_cpu.h>
#endif

namespace Svm {

    using namespace Cache;
    using namespace Memory;

    using PageTable32 = FlatPageTable<u32>;
    using PageTable64 = FlatPageTable<u64>;

    using Dispatcher = DispatcherTable<u64>;

    using VirtualCore = VCpu;

    class BasicBlock {
    public:

        enum State : u8 {
            NONE                = 0,
            REGISTERED_CACHE    = 1 << 0,
            GENERATED_IR        = 1 << 1,
            BOUND_MODULE        = 1 << 2,
            BOUND_DISPATCHER    = 1 << 3,
            COMPILED            = 1 << 4
        };

        struct ModuleInfo {
            u16 module_id{};
            u32 block_id{};
        };

        explicit BasicBlock() {}

        constexpr void SetSize(u16 size) {
            this->size = size;
        }

        FORCE_INLINE void SetIR(SharedPtr<IR::IRBlock> ir) {
            this->ir_block = ir;
            state = static_cast<State>(state | GENERATED_IR);
        }

        constexpr void SetCompiled() {
            state = static_cast<State>(state | COMPILED);
        }

        constexpr bool GetState() {
            return state;
        }

        constexpr void Invalid() {
            if (BoundModule()) {
                state = static_cast<State>(REGISTERED_CACHE | BOUND_MODULE);
            } else {
                state = REGISTERED_CACHE;
            }
            hotness.store(0);
        }

        constexpr bool Registered() {
            return state & REGISTERED_CACHE;
        }

        constexpr bool GeneratedIR() {
            return !ir_block.expired();
        }

        constexpr bool Compiled() {
            return state & COMPILED;
        }

        constexpr bool BoundModule() {
            return state & BOUND_MODULE;
        }

        constexpr u16 IncreaseHotness() {
            return hotness.fetch_add(1);
        }

        constexpr SpinLock &Lock() {
            return lock;
        }

        constexpr void BindModule(u16 module_id, u32 block_id) {
            module_info.module_id = module_id;
            module_info.block_id = block_id;
            state = static_cast<State>(REGISTERED_CACHE | BOUND_MODULE);
        }

        constexpr void BindDispatchTable(u32 index) {
            dispatcher_index = index;
            state = static_cast<State>(REGISTERED_CACHE | BOUND_DISPATCHER);
        }

        constexpr ModuleInfo &GetModule() {
            return module_info;
        };

        constexpr u32 GetDispatchIndex() {
            return dispatcher_index;
        }

        FORCE_INLINE SharedPtr<IR::IRBlock> GetIR() {
            return ir_block.lock();
        }

    public:
        SpinLock lock{};
        Atomic<State> state{NONE};
        u16 size{};
        Atomic<u16> hotness{};
        union {
            u32 dispatcher_index{};
            ModuleInfo module_info;
        };
        WeakPtr<IR::IRBlock> ir_block{};
    };

    struct BlockCacheRef {
        VAddr base;
        BasicBlock *block;
    };

    using BlockCache32 = CodeCache<u32, BasicBlock, 12>;
    using BlockCache64 = CodeCache<u64, BasicBlock, 12>;

    constexpr auto max_cache_modules = 0x1000 / sizeof(void *);

    class JitRuntime : public BaseObject, CopyDisable {
    public:

        explicit JitRuntime(UserConfigs &configs);

        void RegisterCore(VCpu *cpu);

        void UnRegisterCore(VCpu *cpu);

        void Run(VCpu *cpu);

        constexpr Dispatcher &GetDispatcher() {
            return *dispatcher;
        }

        constexpr bool Guest64Bit() {
            return configs.guest_arch == CpuArch::X64 || configs.guest_arch == CpuArch::Arm64;
        }

        constexpr CpuArch GuestArch() {
            return configs.guest_arch;
        }

        constexpr PageTable32 &GetMemory32() {
            return *page_table_32;
        }

        constexpr PageTable64 &GetMemory64() {
            return *page_table_64;
        }

        constexpr BlockCache32 &GetCache32() {
            return *block_cache_32;
        }

        constexpr BlockCache64 &GetCache64() {
            return *block_cache_64;
        }

        constexpr UserConfigs *GetConfigs() {
            return &configs;
        }

        constexpr CowVector<void*> &ModuleBaseTable() {
            return cache_module_bases;
        }

        constexpr CachePool &GetCodeCachePool() {
            return *cache_pool;
        }

        CacheModule *MapModule(VAddr va, u32 size);

        CacheModule *QueryModule(VAddr va);

        CacheModule *GetModule(u16 id);

        void UnMapModule(VAddr va);

        void *FindAndJit(VAddr va, bool fill_dispatcher = false);

        BlockCacheRef GetBlockCache(VAddr addr);

        VAddr GetBlockCodeCache(BasicBlock *cache);

        void PushBlock(VAddr block_start, bool recursive = true);

        void QueueBlockCompile(const BlockCacheRef &cache);

        void InterpreterIRBlock(const BlockCacheRef &cache, VirtualCore *core);

        SharedPtr<IR::IRBlock> NewIRBlock(VAddr base);

        void *CodeCacheTrampoline();

        void *ReturnHostTrampoline();

        void *CallHostTrampoline();

        void *InterruptTrampoline();

    private:

        void EnsureCache(VAddr base, BasicBlock *cache);

        SharedPtr<IR::IRBlock> EnsureIRBlock(const BlockCacheRef &cache, bool recursive = true);

        void RecursiveIRBlock(const SharedPtr<IR::IRBlock>& ir_block);

        void EnsureCompiled(const BlockCacheRef &cache);

        void RunJitCache(CPUContext *context, void *cache);

        Mutex lock;
        UniquePtr<PageTable32> page_table_32;
        UniquePtr<PageTable64> page_table_64;
        UniquePtr<BlockCache32> block_cache_32;
        UniquePtr<BlockCache64> block_cache_64;
        UniquePtr<Dispatcher> dispatcher;
        UniquePtr<JitThread> jit_thread;

        Atomic<u16> current_module_id{};
        UniquePtr<CachePool> cache_pool{};
        Map<VAddr, UniquePtr<CacheModule>> mapped_modules;
        CowVector<void*> cache_module_bases{max_cache_modules};
        LruContainer<IR::IRBlock> ir_blocks_lru{0x10000};

        UserConfigs configs;
        std::set<VCpu*> cores;

        // trampolines
        CPUContext *(*run_code_trampoline)(CPUContext *);
    };

}
