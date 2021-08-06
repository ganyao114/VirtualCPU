//
// Created by swift on 2021/6/23.
//

#include "jit_thread.h"

namespace Svm {

    JitThread::JitThread(const int thread_count) : thread_count(thread_count) {
        for (auto i = 0; i < thread_count; ++i) {
            threads.emplace_back([this] () {
                Looper();
            });
        }
    }

    JitThread::~JitThread() {
        running.store(false, std::memory_order_acquire);
        queue.Stop();
        for (std::thread& thread : threads) {
            thread.join();
        }
    }

    void JitThread::Looper() {
        while (running.load(std::memory_order_acquire)) {
            auto task = queue.Take();
            if (task) {
                task();
            }
        }
    }

    void JitThread::Push(const JitThread::Task &task) {
        queue.Put(task);
    }

}
