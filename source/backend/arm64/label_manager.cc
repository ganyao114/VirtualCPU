//
// Created by SwiftGan on 2020/9/15.
//

#include "label_manager.h"
#include "backend/cpu.h"

namespace Svm::A64 {

#define __ masm_.

    LabelAllocator::LabelAllocator(MacroAssembler &masm) : masm_(masm) {}

    LabelAllocator::~LabelAllocator() {}

    void LabelAllocator::SetDestBuffer(VAddr addr) {
        dest_buffer_start_ = addr;
        assert(dest_buffer_start_);
        for (auto label : labels_outstanding_) {
            ptrdiff_t offset = label.target - dest_buffer_start_;
            __ BindToOffset(label.label, offset);
        }
    }

    Label *LabelAllocator::AllocOutstanding(VAddr target) {
        auto label = AllocLabel();
        labels_outstanding_.push_back({target, label});
        return label;
    }

}