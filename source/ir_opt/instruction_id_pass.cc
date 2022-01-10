//
// Created by 甘尧 on 2022/1/10.
//

#include "instruction_id_pass.h"

namespace Svm::IR {


    void InstrIdPass::Optimize(IRBlock *block, OptResult *result) {
        block->IndexInstructions();
    }

}
