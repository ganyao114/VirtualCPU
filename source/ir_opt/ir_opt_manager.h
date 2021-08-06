//
// Created by swift on 1/8/21.
//

#pragma once

#include <ir/block.h>
#include "ir_opt_result.h"

namespace Svm::IR {

    void OptimizeIR(IRBlock *block, OptResult *result);

}
