//
// Created by swift on 1/11/21.
//

#pragma once

#include "optimize.h"
#include "ir_opt_result.h"

namespace Svm::IR {

    class FlagsGetSetOpt : public IROptimize {
    public:

        void Optimize(IRBlock *block, OptResult *result) override;

    };

}