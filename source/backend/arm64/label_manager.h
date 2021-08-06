//
// Created by SwiftGan on 2020/9/15.
//

#pragma once

#include <aarch64/macro-assembler-aarch64.h>
#include <base/marco.h>
#include <backend/label_manager.h>

namespace Svm::A64 {

    using namespace vixl::aarch64;

    class FatalAddr {
    public:

        constexpr FatalAddr(const Register &rt) : va_{rt} {};

        constexpr FatalAddr(const VAddr &vaddr) : addr_(vaddr), is_const_{true} {};

        const bool Const() const {
            return is_const_;
        }

        const VAddr Address() const {
            return addr_;
        }

        const Register &Reg() const {
            return va_;
        }

    private:
        const bool is_const_{false};
        union {
            Register va_;
            VAddr addr_;
        };
    };

    class LabelAllocator : public BaseLabelManager<Label> {
    public:

        LabelAllocator(MacroAssembler &masm);

        ~LabelAllocator();

        void SetDestBuffer(VAddr addr);

        Label *AllocOutstanding(VAddr target);

    private:

        struct Outstanding {
            VAddr target;
            Label *label;
        };

        VAddr dest_buffer_start_;
        MacroAssembler &masm_;
        std::list<Outstanding> labels_outstanding_;
    };

}
