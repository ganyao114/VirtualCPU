//
// Created by 甘尧 on 2022/1/10.
//

#include "instruction_id_clear.h"

namespace Svm::IR {

    void InstrIdClear::Optimize(IRBlock *block, OptResult *result) {
        block->IndexInstructions();
    }

}
