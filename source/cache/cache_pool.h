//
// Created by swift on 2021/6/22.
//

#pragma once

#include <base/cow_vector.h>
#include <base/simple_heap.h>
#include "code_buffer.h"

namespace Svm::Cache {

    class CachePool {
    public:

        explicit CachePool(u32 max_size);

        CodeBuffer Alloc(u32 size);

        void Free(void *rx_buffer);

        u8 *RwBuffer(void *rx_buffer);

    private:
        SpinMutex lock;
        std::unique_ptr<SimpleHeap<true>> heap;
    };

}
