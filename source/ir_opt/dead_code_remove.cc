//
// Created by SwiftGan on 2021/1/4.
//

#include "dead_code_remove.h"

namespace Svm::IR {

    void DeadCodeRemoveOpt::Optimize(IRBlock *block, OptResult *result) {
        auto &instr_seq = block->Sequence();
        for (auto &instr : instr_seq) {
            if (!HasSideEffect(&instr, result)) {
                instr.Disable();
            }
        }
    }

    bool DeadCodeRemoveOpt::HasSideEffect(Instruction *instr, OptResult *result) {
        if (!instr->GetReturn().IsValue()) {
            return true;
        }
        for (auto use : instr->GetUses()) {
            if (use->Enabled()) {
                return true;
            }
        }
        return false;
    }

}