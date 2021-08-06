//
// Created by swift on 2021/6/23.
//

#pragma once

#include <base/marco.h>
#include <base/blocking_queue.h>
#include <thread>

namespace Svm {

    class JitThread final {
    public:
        using Task = Function<void ()>;

        explicit JitThread(const int thread_count);

        ~JitThread();

        void Push(const Task &task);

    private:

        void Looper();

        const int thread_count;

        AtomicBool running{true};
        Vector<std::thread> threads;
        BlockingQueue<Task> queue;
    };

}
