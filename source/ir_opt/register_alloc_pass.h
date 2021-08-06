//
// Created by swift on 1/9/21.
//

#pragma once

#include "optimize.h"

namespace Svm::IR {

    class RegAllocatePass : public IROptimize {
    public:

        void Optimize(IRBlock *block, OptResult *result) override;

    };

}
