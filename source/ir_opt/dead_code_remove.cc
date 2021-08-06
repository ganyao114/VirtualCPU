//
// Created by SwiftGan on 2021/1/4.
//

#include "dead_code_remove.h"

namespace Svm::IR {

    void DeadCodeRemoveOpt::Optimize(IRBlock *block, OptResult *result) {
        auto &instr_seq = block->Sequence();
        for (auto instr : instr_seq) {
            if (!HasSideEffect(instr, result)) {
                result->Disable(instr->GetId());
            }
        }
    }

    bool DeadCodeRemoveOpt::HasSideEffect(Instruction *instr, OptResult *result) {
        if (!instr->GetReturn().IsValue()) {
            return true;
        }
        for (auto id : instr->GetUses()) {
            if (result->IsEnable(id)) {
                return true;
            }
        }
        return false;
    }

}