//
// Created by swift on 2021/6/25.
//

#include "ir_translator.h"

#define __ context->Masm().

namespace Svm::A64 {

    void IRCommitA64::Svc(IR::Imm &svc_num) {
        Exception::Action action{Exception::SVC};
        auto &tmp = RTemp();
        __ Mov(tmp, svc_num.data);
        context->RaiseException(action, context->PC(), tmp);
        Release(tmp);
    }

    void IRCommitA64::Brk(IR::Imm &brk_num) {
        Exception::Action action{Exception::BRK};
        auto &tmp = RTemp();
        __ Mov(tmp, brk_num.data);
        context->RaiseException(action, context->PC(), tmp);
        Release(tmp);
    }

    void IRCommitA64::Hlt(IR::Imm &hlt_number) {
        Exception::Action action{Exception::HLT};
        auto &tmp = RTemp();
        __ Mov(tmp, hlt_number.data);
        context->RaiseException(action, context->PC(), tmp);
        Release(tmp);
    }

    void IRCommitA64::Yield() {
        Exception::Action action{Exception::YIELD};
        auto &tmp = RTemp();
        context->RaiseException(action, context->PC(), tmp);
        Release(tmp);
    }

}