//
// Created by SwiftGan on 2020/9/15.
//

#pragma once

#include <aarch64/macro-assembler-aarch64.h>
#include <base/marco.h>
#include <backend/label_manager.h>

namespace Svm::A64 {

    using namespace vixl::aarch64;

    class FatalAddress {
    public:

        constexpr FatalAddress(const Register &rt) : va_{rt} {};

        constexpr FatalAddress(const VAddr &vaddr) : addr_(vaddr), is_const_{true} {};

        [[nodiscard]] bool Const() const {
            return is_const_;
        }

        [[nodiscard]] VAddr Address() const {
            return addr_;
        }

        [[nodiscard]] const Register &Reg() const {
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

    class RuntimeLabels {
    public:
        virtual Label *Dispatcher() = 0;

        virtual Label *BlockLabel(VAddr vaddr) = 0;

        virtual Label *LinkLabel(VAddr link_target) = 0;
    };

}
