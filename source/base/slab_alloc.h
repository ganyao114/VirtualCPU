#pragma once

#include <cstdlib>
#include <atomic>
#include <cassert>

namespace Svm {

    class SlabAllocator {
    public:
        struct Node {
            Node *next{};
        };

        constexpr SlabAllocator() = default;

        void Initialize(size_t size) {
            size_ = size;
        }

        constexpr std::size_t GetSize() const {
            return size_;
        }

        Node *GetHead() const {
            return head_;
        }

        void *Allocate();

        void Free(void *obj);

    private:
        std::atomic<Node *> head_{};
        size_t size_{};
    };

    template<typename T>
    class SlabHeap {
    public:
        constexpr SlabHeap() = default;

        constexpr SlabHeap(size_t count) {
            slab_memory = malloc(sizeof(T) * count);
            Initialize(slab_memory, sizeof(T) * count);
        }

        constexpr ~SlabHeap() {
            if (slab_memory) {
                free(slab_memory);
            }
        }

        constexpr bool Contains(uintptr_t addr) const {
            return start <= addr && addr < end;
        }

        constexpr std::size_t GetSlabHeapSize() const {
            return (end - start) / GetObjectSize();
        }

        constexpr std::size_t GetObjectSize() const {
            return allocator_.GetSize();
        }

        std::size_t GetObjectIndexImpl(const void *obj) const {
            return (reinterpret_cast<uintptr_t>(obj) - start) / GetObjectSize();
        }

        template<typename ...Args>
        T *New(Args... args) {
            T *obj = Allocate();
            if (obj != nullptr) {
                new(obj) T(args...);
            }
            return obj;
        }

        void Delete(T *obj) {
            obj->~T();
            Free(obj);
        }

        T *Allocate() {
            T *obj = static_cast<T *>(allocator_.Allocate());
            return obj;
        }

        void Free(T *obj) {
            assert(Contains(reinterpret_cast<uintptr_t>(obj)));
            allocator_.Free(obj);
        }

        void Initialize(void *memory, size_t memory_size) {
            assert(memory != nullptr);

            auto object_size = sizeof(T);

            allocator_.Initialize(object_size);

            const std::size_t num_obj = (memory_size / object_size);
            start = reinterpret_cast<uintptr_t>(memory);
            end = start + num_obj * object_size;

            auto *cur = reinterpret_cast<uint8_t *>(end);

            for (std::size_t i{}; i < num_obj; i++) {
                cur -= object_size;
                allocator_.Free(cur);
            }
        }

    private:

        SlabAllocator allocator_;
        uintptr_t start{};
        uintptr_t end{};
        void *slab_memory{};
    };

    template<class T>
    class SlabObject {
    private:
        static inline SlabHeap<T> slab_heap_;
    public:
        constexpr SlabObject() {}

        static void InitializeSlabHeap(void *memory, size_t memory_size) {
            slab_heap_.Initialize(memory, memory_size);
        }

        constexpr static T *Allocate() {
            return slab_heap_.Allocate();
        }

        constexpr static void Free(T *obj) {
            slab_heap_.Free(obj);
        }

        template<typename ...Args>
        constexpr static T *New(Args... args) {
            return slab_heap_.New(args...);
        }

        constexpr static void Delete(T *obj) {
            slab_heap_.Delete(obj);
        }
    };

#define DEFINE_SLAB(Clazz) \
    void *operator new(size_t sz) { \
        return slab_heap_.Allocate(); \
    } \
    void operator delete(void *p) { \
        slab_heap_.Free(static_cast<Clazz *>(p));\
    }

}