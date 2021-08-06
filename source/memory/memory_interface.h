//
// Created by SwiftGan on 2020/10/20.
//

#pragma once

#include <base/marco.h>
#include "stdexcept"

namespace Svm::Memory {

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

    template <typename Addr>
    class MemoryInterface {
    public:
        virtual void ReadMemory(const Addr src_addr, void *dest_buffer, const std::size_t size) = 0;

        virtual void WriteMemory(const Addr dest_addr, const void* src_buffer, const std::size_t size) = 0;

        virtual std::optional<void *> GetPointer(Addr vaddr) = 0;

        template <typename T>
        T Read(Addr vaddr) {
            T t;
            ReadMemory(vaddr, &t, sizeof(T));
            return std::move(t);
        }

        template <typename T>
        void Write(Addr vaddr, const T &t) {
            WriteMemory(vaddr, &t, sizeof(T));
        }

        template <typename T>
        T *Get(Addr vaddr) {
            auto point = GetPointer(vaddr);
            if (point) {
                return reinterpret_cast<T*>(*point);
            } else {
                return nullptr;
            }
        }

        template <typename T>
        bool AtomicCompareAndSwap(Addr vaddr, T value, T expected) {
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
