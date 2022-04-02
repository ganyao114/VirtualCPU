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
        using Task = std::function<void ()>;

        explicit JitThread(int thread_count);

        ~JitThread();

        void Push(const Task &task);

    private:

        void Looper();

        const int thread_count;

        std::atomic_bool running{true};
        std::vector<std::thread> threads;
        BlockingQueue<Task> queue;
    };

}
