//
// Created by swift on 2021/5/13.
//

#include "cache_module.h"
#include <externals/vixl/aarch64/instructions-aarch64.h>

namespace Svm::Cache {

    constexpr u32 MAX_CACHE_LENGTH = 1024 * 1024;
    constexpr u32 CACHE_END_MAGIC = 0x12345678;

    FORCE_INLINE static u32 BranchOffsetA64(s32 offset) {
        return 0x14000000 | (0x03ffffff & (static_cast<u32>(offset) >> 2));
    }

    FORCE_INLINE static void *GetBranchOffsetTargetA64(u32 instr) {
        auto instruction = reinterpret_cast<vixl::aarch64::Instruction *>(instr);
        return (void *) instruction->GetImmPCOffsetTarget();
    }

    FORCE_INLINE static u32 GetCacheSize(u32 *start) {
        for (u32 i = 0; i < MAX_CACHE_LENGTH / sizeof(u32); ++i) {
            if (start[i] == CACHE_END_MAGIC) {
                return (i + 1) * sizeof(u32);
            }
        }
        return 0;
    }

    ModuleTable::ModuleTable(CowVector<u8, true> *memory, u8 *base) : memory(memory), base(base) {}

    void *ModuleTable::BindCache(u32 index, void *cache) {
#if defined(__aarch64__)
        auto jump_rx = reinterpret_cast<u32 *>(base);
        auto jump_rw = reinterpret_cast<u32 *>(memory->GetRW(base));
        auto item_rx = &jump_rx[index];
        auto item_rw = &jump_rw[index];
        s32 offset = reinterpret_cast<VAddr>(cache) - reinterpret_cast<VAddr>(item_rx);
        *item_rw = BranchOffsetA64(offset);
        __sync_synchronize();
        Platform::ClearDCache(reinterpret_cast<VAddr>(item_rw), 4);
        Platform::ClearICache(reinterpret_cast<VAddr>(item_rx), 4);
        Platform::ClearICache(reinterpret_cast<VAddr>(item_rw), 4);
        return item_rx;
#endif
        UNREACHABLE();
    }

    void ModuleTable::UnBindCache(u32 index) {
        BindCache(index, stub);
    }

    CodeBuffer ModuleTable::GetBoundCache(u32 index) {
#if defined(__aarch64__)
        auto jump_rx = reinterpret_cast<u32 *>(base);
        auto jump_rw = reinterpret_cast<u32 *>(memory->GetRW(base));
        auto item_rx = &jump_rx[index];
        auto item_rw = &jump_rw[index];
        auto instr = *item_rx;
        if (!instr) {
            return {};
        }
        auto target = GetBranchOffsetTargetA64(instr);
        if (target == stub) {
            return {};
        }
        auto cache_size = GetCacheSize(reinterpret_cast<u32 *>(target));
        return {(u8*)item_rx, (u8*)target, (u8*)item_rw, cache_size};
#endif
        UNREACHABLE();
    }

    void ModuleTable::SetStub(u8 *stub) {
        this->stub = stub;
    }

    u8 *ModuleTable::GetStubData(u32 index) {
#if defined(__aarch64__)
        auto jump_rx = reinterpret_cast<u32 *>(base);
        return reinterpret_cast<u8 *>(&jump_rx[index]);
#endif
        UNREACHABLE();
    }

    CacheModule::CacheModule(u16 id, VAddr base, u32 size) : id(id), size(size), module_base(base) {
        max_index = (size >> 2) / 32;
        heap = std::make_unique<SimpleHeap<true>>(size);
        u32 jump_entry_size{};
#if defined(__aarch64__)
        jump_entry_size = 4;
#elif defined(__x86_64__)
        jump_entry_size = 4;
#endif
        auto jump_table_mem = heap->Malloc(jump_entry_size * max_index);
        auto jump_mem_rx = heap->Memory().GetRX(static_cast<u8 *>(jump_table_mem));
        module_table = std::make_unique<ModuleTable>(&heap->Memory(), jump_mem_rx);
    }

    CodeBuffer CacheModule::AllocBuffer(u32 index, u32 size) {
        std::lock_guard guard(lock);
        size = AlignUp(size, 4) + 4;
        auto rw = reinterpret_cast<u8 *>(heap->Malloc(size));
        *reinterpret_cast<u32 *>(rw + size - 4) = CACHE_END_MAGIC;
        auto rx = heap->Memory().GetRX(rw);
        auto stub_rx = module_table->GetStubData(index);
#if defined(__aarch64__)
        u32 *stub_instr = reinterpret_cast<u32 *>(rw);
        // Loop
        stub_instr[0] = BranchOffsetA64(0);
        Platform::ClearDCache(reinterpret_cast<VAddr>(rw), 4);
        Platform::ClearDCache(reinterpret_cast<VAddr>(rx), 4);
        Platform::ClearICache(reinterpret_cast<VAddr>(rx), 4);
#endif
        module_table->BindCache(index, rx);
        return {(u8*)rx, (u8*)stub_rx, (u8*)rw, size};
    }

    void CacheModule::BindForwardStub(u8 *stub) {
        to_stub_cache = stub;
        module_table->SetStub(stub);
    }

    void CacheModule::ClearBufferIndex(u32 index) {
        std::lock_guard guard(lock);
        auto buffer = module_table->GetBoundCache(index);
        if (buffer.size) {
            heap->Free(buffer.rw_data);
            module_table->UnBindCache(index);
        }
    }

    CodeBuffer CacheModule::GetBoundBuffer(u32 index) {
        return module_table->GetBoundCache(index);
    }

    u32 CacheModule::AllocId() {
        return current_index.fetch_add(1);
    }

}
