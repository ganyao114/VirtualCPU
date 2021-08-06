//
// Created by SwiftGan on 2020/12/28.
//

#pragma once

#include <queue>
#include "marco.h"

namespace Svm {

    #define MAX_CAPACITY 10000

    template<typename T>
    class BlockingQueue {
    public:

        explicit BlockingQueue(size_t capacity = MAX_CAPACITY) : mtx(), full_(), empty_(), capacity_(capacity) {}

        void Put(const T& task) {
            std::unique_lock<std::mutex> lock(mtx);
            while(queue_.size() == capacity_ && !stopped){
                full_.wait(lock );
            }
            assert(queue_.size() < capacity_);
            queue_.push(task);
            empty_.notify_all();
        }

        T Take() {
            std::unique_lock<std::mutex> lock(mtx);
            while(queue_.empty() && !stopped){
                empty_.wait(lock );
            }
            if (stopped) return {};
            assert(!queue_.empty());
            T front(queue_.front());
            queue_.pop();
            full_.notify_all();
            return std::move(front);
        }

        T Front() {
            std::unique_lock<std::mutex> lock(mtx);
            while(queue_.empty()){
                empty_.wait(lock );
            }
            assert(!queue_.empty());
            T front(queue_.front());
            return front;
        }

        T Back() {
            std::unique_lock<std::mutex> lock(mtx);
            while(queue_.empty() && !stopped){
                empty_.wait(lock);
            }
            if (stopped) return {};
            assert(!queue_.empty());
            T back(queue_.back());
            return back;
        }

        size_t Size() {
            std::lock_guard<std::mutex> lock(mtx);
            return queue_.size();
        }

        bool Empty() {
            std::unique_lock<std::mutex> lock(mtx);
            return queue_.empty();
        }

        void SetCapacity(const size_t capacity) {
            capacity_ = (capacity > 0 ? capacity : MAX_CAPACITY);
        }

        void Stop() {
            std::unique_lock<std::mutex> lock(mtx);
            stopped = true;
            empty_.notify_all();
            full_.notify_all();
        }

    private:
        BlockingQueue(const BlockingQueue& rhs);
        BlockingQueue& operator= (const BlockingQueue& rhs);

    private:
        mutable std::mutex mtx;
        std::condition_variable full_;
        std::condition_variable empty_;
        std::queue<T> queue_;
        size_t capacity_;
        std::atomic_bool stopped{false};
    };

}
