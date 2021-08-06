//
// Created by SwiftGan on 2020/9/8.
//

#include "marco.h"

namespace Svm {

    static s64 GetThreadId() {
        return reinterpret_cast<s64>(pthread_self());
    }

#ifdef USE_STD_CPP

#include <unistd.h>

    void SpinMutex::Lock() {
        s64 expected = -1;
        s64 tid = GetThreadId();
        while (!flag.compare_exchange_strong(expected, tid)) {
            expected = -1;
            asm("yield");
        }
    }

    bool SpinMutex::TryLock() {
        s64 expected = -1;
        s64 tid = GetThreadId();
        return flag.compare_exchange_strong(expected, tid);
    }

    bool SpinMutex::LockedBySelf() {
        return flag == GetThreadId();
    }

    void SpinMutex::LockRecursive() {
        if (LockedBySelf()) {
            recursive++;
        } else {
            Lock();
            __sync_synchronize();
            recursive = 1;
        }
    }

    void SpinMutex::UnlockRecursive() {
        if (LockedBySelf()) {
            if ((--recursive) == 0) {
                Unlock();
            }
        }
    }

    bool SpinMutex::LastRecursive() {
        return LockedBySelf() && recursive == 1;
    }

#endif

    SpinLockGuard::SpinLockGuard(SpinMutex &mutex) : mutex_(mutex) {
        mutex_.Lock();
    }

    SpinLockGuard::~SpinLockGuard() {
        mutex_.Unlock();
    }

    void SpinLockGuard::Lock() {
        mutex_.Lock();
    }

    void SpinLockGuard::Unlock() {
        mutex_.Unlock();
    }

}
