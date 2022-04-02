//
// Created by 甘尧 on 2022/3/17.
//

#include "context_promotion_pass.h"

namespace Svm::IR {

    void CtxPromotionPass::Optimize(IRBlock *block, OptResult *result) {
        auto &instr_seq = block->Sequence();
        for (auto &instr : instr_seq) {

        }
    }

}