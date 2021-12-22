//
// Created by swift on 2021/5/26.
//

#pragma once

#include <cstdint>
#include <array>
#include <map>
#include <vector>
#include <list>
#include <set>
#include <stack>
#include <unordered_map>

namespace Svm {

#define OFFSET_OF(t, d) __builtin_offsetof(t, d)
#define FORCE_INLINE inline __attribute__((always_inline))
#define ALWAYS_INLINE  __attribute__ ((always_inline))

    constexpr auto kBitsPerByteLog2 = 3;
    constexpr auto kBitsPerByte = 1 << 3;
    constexpr auto KB = 1024;
    constexpr auto MB = KB * KB;
    constexpr auto GB = KB * KB * KB;

    using u8 = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;
    using u128 = std::array<u64, 2>;

    using s8 = std::int8_t;
    using s16 = std::int16_t;
    using s32 = std::int32_t;
    using s64 = std::int64_t;

    using f32 = float;
    using f64 = double;
    using f128 = u128;

    using VAddr = u64;

    using PAddr = u64;

    template<typename T, size_t size>
    using Array = std::array<T, size>;

    template<typename K, typename V>
    using Map = std::map<K, V>;

    template<typename T>
    using Stack = std::stack<T>;

    template<typename K, typename V>
    using UnorderedMap = std::unordered_map<K, V>;

    template<typename T>
    using Vector = std::vector<T>;

    template<typename T>
    using List = std::list<T>;

    template<typename T>
    using Set = std::set<T>;

    enum class CpuArch {
        Ukn,
        Arm32,
        Arm64,
        X86,
        X64
    };

    class CopyDisable {
    protected:
        constexpr CopyDisable() = default;

        ~CopyDisable() = default;

        CopyDisable(const CopyDisable &) = delete;

        CopyDisable &operator=(const CopyDisable &) = delete;
    };

    template<std::size_t position, std::size_t bits, typename T>
    struct BitField {
    public:
        BitField() = default;

        FORCE_INLINE BitField &operator=(T val) {
            storage = (storage & ~mask) | ((val << position) & mask);
            return *this;
        }

        FORCE_INLINE operator T() const {
            return (T)((storage & mask) >> position);
        }

    private:
        BitField(T val) = delete;

        T storage;

        static constexpr T mask = ((~(T) 0) >> (8 * sizeof(T) - bits)) << position;

        static_assert(!std::numeric_limits<T>::is_signed);
        static_assert(bits > 0);
    };
}
