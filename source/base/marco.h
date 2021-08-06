//
// Created by SwiftGan on 2020/9/8.
//

#pragma once

#define USE_STD_CPP 1

#include "include/types.h"
#include "assert.h"

#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <bitset>
#include <queue>
#include <type_traits>

#define FIELD(type, name, from, to) \
        struct { \
            type : (from); \
            type name : ((to) - (from) + 1); \
            type : (sizeof(type) * 8 - 1 - (to)); \
        }

#define FIELD32(name, from, to) FIELD(u32, name, from, to)
#define FIELD64(name, from, to) FIELD(u64, name, from, to)
#define FIELD16(name, from, to) FIELD(u16, name, from, to)

#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#define CONCAT2(x, y) DO_CONCAT2(x, y)
#define DO_CONCAT2(x, y) x##y

#define PADDING_BYTES(num_bytes) u8 CONCAT2(unk, __LINE__)[(num_bytes)]
#define PADDING_WORDS(num_words) u32 CONCAT2(unk, __LINE__)[(num_words)]
#define UNION_PADDING_BYTES(num_bytes) std::array<u8, num_bytes> CONCAT2(unk, __LINE__)
#define UNION_PADDING_WORDS(num_words) std::array<u32, num_words> CONCAT2(unk, __LINE__)

#define DCHECK(x)
#define CHECK(x)
#define CHECK_OP(x, y, op)
#define CHECK_EQ(x, y) CHECK_OP(x, y, ==)
#define CHECK_NE(x, y) CHECK_OP(x, y, !=)
#define CHECK_LE(x, y) CHECK_OP(x, y, <=)
#define CHECK_LT(x, y) CHECK_OP(x, y, <)
#define CHECK_GE(x, y) CHECK_OP(x, y, >=)
#define CHECK_GT(x, y) CHECK_OP(x, y, >)

namespace Svm {
#ifdef USE_STD_CPP

#define ASSERT(exp) assert(exp)
#define UNREACHABLE() abort()
#define ABORT() abort()

    using Mutex = std::mutex;

    using RecursiveMutex = std::recursive_mutex;

    using SharedMutex = std::shared_mutex;

    using String = std::string;

    using Duration = std::chrono::steady_clock::duration;

    using TimePoint = std::chrono::steady_clock::time_point;

    using Ns = std::chrono::nanoseconds;

    using Ms = std::chrono::milliseconds;

    template<unsigned bits>
    using BitSet = std::bitset<bits>;

    template<typename T>
    using Queue = std::queue<T>;

    template<typename T>
    using Atomic = std::atomic<T>;

    using AtomicBool = std::atomic_bool;

    using LockGuard = std::lock_guard<std::mutex>;

    using RecursiveGuard = std::lock_guard<std::recursive_mutex>;

    template<typename T = SharedMutex>
    using UniqueLock = std::unique_lock<T>;

    template<typename T = SharedMutex>
    using SharedLock = std::shared_lock<T>;

    template<typename T = void()>
    using Function = std::function<T>;

    class BaseObject : public std::enable_shared_from_this<BaseObject> {
    };

    template<typename T>
    using SharedPtr = std::shared_ptr<T>;

    template<typename T>
    using WeakPtr = std::weak_ptr<T>;

    template<typename T>
    using UniquePtr = std::unique_ptr<T>;

    template<typename T>
    using Optional = std::optional<T>;

    template<typename T1, typename T2>
    using Pair = std::pair<T1, T2>;


    template<typename T>
    FORCE_INLINE std::shared_ptr<T> DynamicCast(std::shared_ptr<BaseObject> object) {
        if (object != nullptr) {
            return std::static_pointer_cast<T>(object);
        }
        return nullptr;
    }

    template<typename T, typename ...Args>
    constexpr std::shared_ptr<T> MakeShared(Args... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template<typename T, typename ...Args>
    constexpr std::unique_ptr<T> MakeUnique(Args... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    FORCE_INLINE std::shared_ptr<T> SharedFrom(T *raw) {
        if (raw == nullptr)
            return nullptr;
        return std::static_pointer_cast<T>(raw->shared_from_this());
    }

#endif

    template<typename T>
    struct Rect {
        T left{};
        T top{};
        T right{};
        T bottom{};
    };

    class SpinMutex {
    public:
        SpinMutex() = default;

        void Lock();

        void LockRecursive();

        inline void Unlock() {
            flag.store(-1);
        }

        void UnlockRecursive();

        bool LastRecursive();

        bool TryLock();

        bool LockedBySelf();

        constexpr bool IsLocked() {
            return flag != -1;
        }

        constexpr bool CanLock() {
            return flag == -1;
        }

    private:
        Atomic<s64> flag{-1};
        Atomic<int> recursive{1};
    };

    class SpinGuard {
    public:

        explicit SpinGuard(SpinMutex &mutex) : mutex_{mutex} {
            mutex_.LockRecursive();
        }

        FORCE_INLINE ~SpinGuard() {
            mutex_.UnlockRecursive();
        }

    private:
        SpinMutex &mutex_;
    };

    class ExitGuard {
    public:

        inline explicit ExitGuard(Function<void(void)> exit) : exit_{exit} {}

        inline ~ExitGuard() {
            if (!canceled_) {
                exit_();
            }
        }

        constexpr void Cancel() {
            canceled_ = true;
        }

    private:
        Function<void(void)> exit_;
        bool canceled_{false};
    };

    // Like sizeof, but count how many bits a type takes. Pass type explicitly.
    template<typename T>
    constexpr size_t BitSizeOf() {
        static_assert(std::is_integral<T>::value, "T must be integral");
        using unsigned_type = typename std::make_unsigned<T>::type;
        static_assert(sizeof(T) == sizeof(unsigned_type), "Unexpected type size mismatch!");
        static_assert(std::numeric_limits<unsigned_type>::radix == 2, "Unexpected radix!");
        return std::numeric_limits<unsigned_type>::digits;
    }

// Like sizeof, but count how many bits a type takes. Infers type from parameter.
    template<typename T>
    constexpr size_t BitSizeOf(T /*x*/) {
        return BitSizeOf<T>();
    }

    template<typename T>
    constexpr T GetIntLimit(size_t bits) {
        return static_cast<T>(1) << (bits - 1);
    }

    template<size_t kBits, typename T>
    constexpr bool IsInt(T value) {
        static_assert(kBits > 0, "kBits cannot be zero.");
        static_assert(kBits <= BitSizeOf<T>(), "kBits must be <= max.");
        static_assert(std::is_signed<T>::value, "Needs a signed type.");
        // Corner case for "use all bits." Can't use the limits, as they would overflow, but it is
        // trivially true.
        return (kBits == BitSizeOf<T>()) ?
               true :
               (-GetIntLimit<T>(kBits) <= value) && (value < GetIntLimit<T>(kBits));
    }

    template<size_t kBits, typename T>
    constexpr bool IsUint(T value) {
        static_assert(kBits > 0, "kBits cannot be zero.");
        static_assert(kBits <= BitSizeOf<T>(), "kBits must be <= max.");
        static_assert(std::is_integral<T>::value, "Needs an integral type.");
        // Corner case for "use all bits." Can't use the limits, as they would overflow, but it is
        // trivially true.
        // NOTE: To avoid triggering assertion in GetIntLimit(kBits+1) if kBits+1==BitSizeOf<T>(),
        // use GetIntLimit(kBits)*2u. The unsigned arithmetic works well for us if it overflows.
        using unsigned_type = typename std::make_unsigned<T>::type;
        return (0 <= value) &&
               (kBits == BitSizeOf<T>() ||
                (static_cast<unsigned_type>(value) <= GetIntLimit<unsigned_type>(kBits) * 2u - 1u));
    }

    template<typename T>
    constexpr T AlignUp(T value, std::size_t size) {
        return static_cast<T>(value + (size - value % size) % size);
    }

    template<typename T>
    constexpr T AlignDown(T value, std::size_t size) {
        return static_cast<T>(value - value % size);
    }

    constexpr u32 MakeMagic(char a, char b, char c, char d) {
        return u32(a) | u32(b) << 8 | u32(c) << 16 | u32(d) << 24;
    }

    constexpr u8 HexToByte(char ch) {
        if (ch >= '0' && ch <= '9')
            return ch - '0';
        else if (ch >= 'a' && ch <= 'f')
            return ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'F')
            return ch - 'A' + 10;
        return ch;
    }

    template<std::size_t Size, bool le = false>
    constexpr Array<u8, Size> HexStringToArray(std::string_view str) {
        Array<u8, Size> out{};
        if constexpr (le) {
            for (std::size_t i = 2 * Size - 2; i <= 2 * Size; i -= 2)
                out[i / 2] = (HexToByte(str[i]) << 4) | HexToByte(str[i + 1]);
        } else {
            for (std::size_t i = 0; i < 2 * Size; i += 2)
                out[i / 2] = (HexToByte(str[i]) << 4) | HexToByte(str[i + 1]);
        }
        return out;
    }

    constexpr u32 Low32Bits(u64 value) {
        return static_cast<u32>(value);
    }

    constexpr u32 High32Bits(u64 value) {
        return static_cast<u32>(value >> 32);
    }

    template<typename T>
    constexpr T BitClear(T data, int from, int to) {
        T mask = ~(((T) 1 << (to + 1)) - 1);
        mask |= ((T) 1 << from) - 1;
        return data & mask;
    }

    FORCE_INLINE String BufferToString(const Vector<u8> &data) {
        return String(data.begin(), std::find(data.begin(), data.end(), '\0'));
    }

    template<class D, class S>
    constexpr D BitCast(const S &source) {
        static_assert(sizeof(D) == sizeof(S), "sizes should be equal");
        D dest;
        memcpy(&dest, &source, sizeof(dest));
        return std::move(dest);
    }

#pragma pack()

    struct Vector3f {
        float x;
        float y;
        float z;

        constexpr float GetLength() const {
            return x * x + y * y + z * z;
        }
    };

    struct Quaternion {
        float x;
        float y;
        float z;
        float w;
    };

    class SpinLockGuard {
    public:
        SpinLockGuard(SpinMutex &mutex);

        ~SpinLockGuard();

        void Lock();

        void Unlock();

    private:
        SpinLockGuard(SpinLockGuard const &) = delete;

        SpinLockGuard &operator=(SpinLockGuard const &) = delete;

        SpinMutex &mutex_;
    };

    FORCE_INLINE void Yield() {
        asm("yield");
    }

    class SpinLock {
    public:

        FORCE_INLINE void lock() {
            while (__sync_lock_test_and_set(&is_held, true)) {
                Yield();
            }
        }

        FORCE_INLINE void unlock() {
            __sync_lock_release(&is_held);
        }

    private:
        volatile bool is_held{false};
    };

    using SpinScope = std::lock_guard<SpinLock>;

    template<typename T>
    constexpr bool IsNegate(T result, unsigned value_size) {
        return (result >> (value_size - 1)) & 1;
    }

    template<typename T>
    constexpr bool IsZero(T result) { return result == 0; }

}

