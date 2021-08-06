//
// Created by swift on 2021/5/13.
//

#pragma once

#include "code_cache.h"
#include <base/simple_heap.h>
#include "code_buffer.h"

namespace Svm::Cache {

    class ModuleTable {
    public:

        ModuleTable(CowVector<u8, true> *memory, u8 *base);

        void *BindCache(u32 index, void *cache);

        void SetStub(u8 *stub);

        CodeBuffer GetBoundCache(u32 index);

        u8 *GetStubData(u32 index);

        void UnBindCache(u32 index);

    private:
        u8 *base;
        u8 *stub;
        CowVector<u8, true> *memory;
    };

    class CacheModule : public BaseObject {
    public:

        struct SlowBuffer {
            u8 *exec{};
            u32 size{};
        };

        CacheModule(u16 id, VAddr base, u32 size);

        void BindForwardStub(u8 *stub);

        u32 AllocId();

        CodeBuffer GetBoundBuffer(u32 index);

        CodeBuffer AllocBuffer(u32 index, u32 size);

        void ClearBufferIndex(u32 index);

        constexpr u16 GetModuleId() {
            return id;
        }

        constexpr bool Overlap(VAddr va) {
            return va >= module_base && va <= (module_base + size);
        }

        constexpr bool Full() {
            return false;
        }

    private:
        u16 id;
        SpinMutex lock;
        UniquePtr<ModuleTable> module_table;
        UniquePtr<SimpleHeap<true>> heap;
        u8 *to_stub_cache;
        Atomic<u32> current_index{};
        u32 max_index;
        VAddr module_base;
        u32 size;
    };

}
