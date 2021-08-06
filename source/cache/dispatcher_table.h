//
// Created by SwiftGan on 2020/9/15.
//

#pragma once

#include <base/marco.h>
#include <base/cow_vector.h>

namespace Svm::Cache {

#define HASH_TABLE_PAGE_BITS 22

    template<typename Addr>
    class DispatcherTable : public BaseObject {
    public:

        struct Entry {
            Addr key;
            VAddr value;
        };

        DispatcherTable() {
            size = 1 << HASH_TABLE_PAGE_BITS;
            entries.Resize(size + 10);
        }

        constexpr Entry *Data() {
            return entries.DataRW();
        }

        u32 Hash(Addr key) {
            Addr merged = key >> 2;
            return (merged >> HASH_TABLE_PAGE_BITS ^ merged) & (size - 1);
        }

        size_t GetCollisionCount() const {
            return collisions;
        }

        u32 Put(Addr key, VAddr value) {
            u32 index = Hash(key);
            bool done = false;

            SpinLockGuard guard(lock);
            do {
                if (entries[index].key == 0 || entries[index].key == key) {
                    if (entries[index].key == 0) {
                        count++;
                    }
                    entries[index].value = value;
                    std::atomic_thread_fence(std::memory_order_acquire);
                    entries[index].key = key;
                    done = true;
                } else {
                    index++;
                    if (index >= size - 1) {
                        abort();
                    }
                    collisions++;
                }
            } while (!done && index < (size - 1));

            if (done) {
                return index;
            } else {
                return {};
            }
        }

        VAddr Get(Addr key) {
            u32 index = Hash(key);
            bool found = false;
            VAddr entry = 0;
            VAddr c_key;

            SpinLockGuard guard(lock);
            do {
                c_key = entries[index].key;
                if (c_key == key) {
                    entry = entries[index].value;
                    found = true;
                } else {
                    index++;
                }
            } while (!found && index < (size - 1) && c_key != 0);
            return entry;
        }

        Entry &GetEntry(u32 index) {
            return entries[index];
        }

        void Remove(Addr key) {
            u32 index = Hash(key);
            u32 end = index - 1;
            bool found = false;
            Addr c_key;

            SpinLockGuard guard(lock);
            do {
                c_key = entries[index].key;
                if (c_key == key) {
                    entries[index].key = 0;
                    found = true;
                } else {
                    index = (index + 1) & size;
                }
            } while (!found && index != end && c_key != 0);
        }

    private:
        SpinMutex lock{};
        size_t size;
        size_t collisions = 0;
        size_t count = 0;
        CowVector<Entry> entries;
    };

}
