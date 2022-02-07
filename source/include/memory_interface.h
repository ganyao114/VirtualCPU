//
// Created by SwiftGan on 2020/10/20.
//

#pragma once

#include "types.h"
#include "stdexcept"
#include "variant"

namespace Svm::Memory {

    using VAddress = std::variant<u32, u64>;

    class MemoryException : public std::exception {
    public:

        enum Reason : u8 {
            Read,
            Write,
            Exe
        };

        explicit MemoryException(Reason code, VAddr addr) : code(code), addr(addr) {}

        Reason code;
        VAddr addr;
    };

    class MemoryInterface {
    public:
        virtual void ReadMemory(const VAddress &src_addr, void *dest_buffer, size_t size) = 0;

        virtual void WriteMemory(const VAddress &dest_addr, const void* src_buffer, size_t size) = 0;

        virtual std::optional<void *> GetPointer(const VAddress &vaddr) = 0;

        template <typename T>
        T Read(const VAddress &vaddr) {
            T t;
            ReadMemory(vaddr, &t, sizeof(T));
            return std::move(t);
        }

        template <typename T>
        void Write(const VAddress &vaddr, const T &t) {
            WriteMemory(vaddr, &t, sizeof(T));
        }

        template <typename T>
        T *Get(const VAddress &vaddr) {
            auto point = GetPointer(vaddr);
            if (point) {
                return reinterpret_cast<T*>(*point);
            } else {
                return nullptr;
            }
        }

        template <typename T>
        bool AtomicCompareAndSwap(const VAddress &vaddr, T value, T expected) {
            auto volatile phy_point = Get<T>(vaddr);
            if (sizeof(T) == sizeof(u128)) {
                unsigned __int128 value_a;
                unsigned __int128 expected_a;
                std::memcpy(&value_a, &value, sizeof(u128));
                std::memcpy(&expected_a, &expected, sizeof(u128));
                return __sync_bool_compare_and_swap(phy_point, expected_a, value_a);
            } else {
                return __sync_bool_compare_and_swap(phy_point, expected, value);
            }
        }

    };

}
