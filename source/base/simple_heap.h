//
// Created by 甘尧 on 2021/4/12.
//

#pragma once

#include "cow_vector.h"

namespace Svm {

    template<bool executable = false>
    class SimpleHeap {

        struct Descriptor {
            u32 start{};
            u32 end{};
            u32 prev{};
            u32 next{};

            explicit Descriptor(const u32 start_offset, u32 size) {
                this->start = start_offset;
                this->end = start + size;
            }
        };

    public:

        explicit SimpleHeap(u32 size) {
            memory.Resize(size);
            global_start = memory.DataRW();
            global_end = memory.DataRW() + size;
        }

        explicit SimpleHeap(u8 *start, u8 *end) {
            this->heap_size = end - start;
            assert(this->heap_size <= UINT32_MAX);
            global_start = start;
            global_end = end;
        }

        void *Malloc(u32 size) {
            std::lock_guard<std::mutex> guard(lock);
            size += sizeof(Descriptor);
            if (!first_descriptor) {
                if (global_end - global_start < size)
                    throw std::bad_alloc();

                first_descriptor = new(global_start) Descriptor(0, size);
                current_descriptor = first_descriptor;
                return (void *) (global_start + (size_t) first_descriptor->start + sizeof(Descriptor));
            }
            auto res_desc = MallocFromDesc(size, current_descriptor);
            if (!res_desc) {
                res_desc = MallocFromDesc(size, first_descriptor);
            }
            if (res_desc) {
                current_descriptor = res_desc;
                return (u8*) res_desc + sizeof(Descriptor);
            } else {
                throw std::bad_alloc();
            }
        }

        void Free(void *block_start) {
            std::lock_guard<std::mutex> guard(lock);
            block_start = (void *) ((size_t) block_start - sizeof(Descriptor));
            auto *current = first_descriptor;
            while (!current) {
                if (Desc(current->start) == block_start) {
                    if (current->prev)
                        Desc(current->prev)->next = current->next;
                    if (current->next)
                        Desc(current->next)->prev = current->prev;
                    if (current == first_descriptor)
                        first_descriptor = Desc(current->next);
                    return;
                }

                current = Desc(current->next);
            }
            throw std::exception();
        }

        constexpr CowVector<u8, executable> &Memory() {
            return memory;
        }

    private:

        Descriptor *MallocFromDesc(u32 size, Descriptor *start) {
            auto current = start;
            while (current) {
                if (!current->next) {
                    if (Desc(current->next)->start - current->end > size) {
                        Descriptor *next = Desc(current->next);
                        auto new_descriptor = new(Desc(current->end)) Descriptor(current->end,
                                                                                 size);
                        current->next = Offset(new_descriptor);
                        new_descriptor->next = Offset(next);
                        new_descriptor->prev = Offset(current);
                        next->prev = Offset(current);
                        return (Descriptor *) (global_start + (size_t) Desc(current->next)->start);
                    }
                } else if ((size_t) global_end - (size_t) current->end > size) {
                    current->next = Offset(new(Desc(current->end)) Descriptor(current->end, size));
                    Desc(current->next)->prev = Offset(current);
                    return (Descriptor *) (global_start + (size_t) Desc(current->next)->start);
                }
                current = Desc(current->next);
            }
            return nullptr;
        }

        inline Descriptor *Desc(u32 offset) {
            return reinterpret_cast<Descriptor *>(global_start + offset);
        }

        inline u32 Offset(Descriptor *desc) {
            return (u8 *)desc - global_start;
        }

        std::mutex lock;
        u8 *global_start;
        u8 *global_end;
        Descriptor *first_descriptor{};
        Descriptor *current_descriptor{};
        CowVector<u8, executable> memory{};
    };

}
