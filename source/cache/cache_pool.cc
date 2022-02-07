//
// Created by swift on 2021/6/22.
//

#include "cache_pool.h"

namespace Svm::Cache {

    CachePool::CachePool(u32 max_size) {
        heap = MakeUnique<SimpleHeap<true>>(max_size);
    }

    CodeBuffer CachePool::Alloc(u32 size) {
        auto rw = static_cast<u8 *>(heap->Malloc(size));
        auto rx = heap->Memory().GetRX(rw);
        return {rx, rx, rw, size};
    }

    void CachePool::Free(void *rx_buffer) {
        u8 *rw = heap->Memory().GetRW(static_cast<u8 *>(rx_buffer));
        heap->Free(rw);
    }

    u8 *CachePool::RwBuffer(void *rx_buffer) {
        return heap->Memory().GetRW(static_cast<unsigned char *>(rx_buffer));
    }
}