//
// Created by swift on 2021/3/20.
//

#pragma once

#include <base/cow_vector.h>

namespace Svm::Cache {

    template<typename T>
    class CacheAllocator {
    public:

        explicit CacheAllocator() {
            block_data.Resize(0x100000);
        }

        T *Alloc() {
            if (last_cursor >= block_data.Size()) {
                return {};
            }
            return &block_data[last_cursor++];
        }

        void Free(T *entry) {
        }

    private:
        CowVector<T> block_data;
        u32 last_cursor{0};
    };

    template<typename Addr, typename T, size_t cache_line, typename Allocator = CacheAllocator<T>>
    class CodeCache {
    public:

        struct CacheLine {
            s16 offset{};
            T* cache{};
        };

        constexpr static auto cache_size = 1 << cache_line;
        constexpr static auto cache_mask = cache_size - 1;

        explicit CodeCache(const bool static_cache) : static_cache(static_cache) {}

        template <typename ...Args>
        T *Emplace(Addr addr, Args... args) {
            {
                // fast path
                SharedLock<SharedMutex> guard(lock);
                auto itr = search_cache.find(addr);
                if (itr != search_cache.end()) {
                    return itr->second;
                }
            }
            {
                UniqueLock<SharedMutex> guard(lock);
                auto itr = search_cache.find(addr);
                if (itr != search_cache.end()) {
                    return itr->second;
                }
                T *cache = AllocEntry();
                ASSERT(cache);
                new(cache) T(std::forward<Args>(args)...);
                search_cache[addr] = cache;
                return cache;
            }
        }

        void Flush(Addr addr, T *entry) {
            if (static_cache) return;
            UniqueLock<SharedMutex> guard(lock);
            const Addr cache_end = (addr + entry->size + cache_size - 1) >> cache_line;
            for (Addr line = addr >> cache_line; line < cache_end; ++line) {
                s16 offset = addr - (line << cache_line);
                invalid_cache[line].push_back({offset, entry});
            }
        }

        void InvalidByCacheLine(Addr addr, size_t size) {
            UniqueLock<SharedMutex> guard(lock);
            const Addr cache_end = (addr + size + cache_size - 1) >> cache_line;
            for (Addr line = addr >> cache_line; line < cache_end; ++line) {
                auto itr = invalid_cache.find(line);
                if (itr == invalid_cache.end()) {
                    continue;
                }
                for (auto &cache : itr->second) {
                    auto base = (line << cache_line) + cache.offset;
                    OnCacheInvalid(base, cache.block);
                }
            }
        }

        void RemoveByCacheLine(Addr addr, size_t size) {
            UniqueLock<SharedMutex> guard(lock);
            const Addr cache_end = (addr + size + cache_size - 1) >> cache_line;
            for (Addr line = addr >> cache_line; line < cache_end; ++line) {
                auto itr = invalid_cache.find(line);
                if (itr == invalid_cache.end()) {
                    continue;
                }
                for (auto &cache : itr->second) {
                    auto base = (line >> cache_line) + cache.offset;
                    OnCacheInvalid(base, cache.block);
                    OnCacheRemoval(cache);
                    search_cache.erase(cache.base);
                    FreeEntry(cache.block);
                    cache.block->~T();
                }
                itr.erase();
            }
        }

        virtual void OnCacheInvalid(Addr base, T *entry) {}

        virtual void OnCacheRemoval(Addr base, T *entry) {}

    private:

        T *AllocEntry() {
            return allocator.Alloc();
        }

        void FreeEntry(T *entry) {
            allocator.Free(entry);
        }

        UnorderedMap<Addr, T*> search_cache{};
        UnorderedMap<Addr, Vector<CacheLine>> invalid_cache{};
        Allocator allocator{};

        const bool static_cache{};
        SharedMutex lock;
    };

}
