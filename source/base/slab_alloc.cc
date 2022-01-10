#include "slab_alloc.h"

namespace Svm {

    void *SlabAllocator::Allocate() {
        Node *ret = head_.load();

        do {
            if (ret == nullptr) {
                break;
            }
        } while (!head_.compare_exchange_weak(ret, ret->next));

        return ret;
    }

    void SlabAllocator::Free(void *obj) {
        Node *node = static_cast<Node *>(obj);

        Node *cur_head = head_.load();
        do {
            node->next = cur_head;
        } while (!head_.compare_exchange_weak(cur_head, node));
    }

}
