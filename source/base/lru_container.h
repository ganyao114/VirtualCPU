//
// Created by swift on 2021/6/25.
//

#pragma once

#include "intrusive_list.hpp"
#include "marco.h"

namespace Svm {

    template<typename T>
    struct LruWrapper {
        intrusive_node node{};
        SharedPtr<T> value{};
    };

    template<typename T>
    class LruContainer {
    public:

        struct Node {
            LruWrapper<T> *lru_owner{};
        };

        explicit LruContainer(size_t max) : max_size(max) {}

        template <typename ...Args>
        SharedPtr<T> New(Args... args) {
            auto res = MakeShared<T>(std::forward<Args>(args)...);
            RecursiveGuard guard(lock);
            auto wrapper = new LruWrapper<T>();
            wrapper->value = res;
            res->lru_owner = wrapper;
            container.push_front(*wrapper);
            if (container.size() > max_size) {
                Delete(container.back().value.get());
            }
            return res;
        }

        void Delete(T *node) {
            LruWrapper<T> *wrapper;
            {
                RecursiveGuard guard(lock);
                if (node->lru_owner) {
                    container.erase(*node->lru_owner);
                    wrapper = node->lru_owner;
                    node->lru_owner = {};
                }
            }
            delete wrapper;
        }

        void Notify(T *node) {
            RecursiveGuard guard(lock);
            if (node->lru_owner) {
                container.erase(*node->lru_owner);
                container.push_front(*node->lru_owner);
            }
        }

    private:
        RecursiveMutex lock;
        intrusive_list<LruWrapper<T>, &LruWrapper<T>::node> container;
        size_t max_size;
    };

}
