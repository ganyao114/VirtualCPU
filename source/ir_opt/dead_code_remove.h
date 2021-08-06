//
// Created by SwiftGan on 2021/1/4.
//

#pragma once

#include "optimize.h"

namespace Svm::IR {

    class DeadCodeRemoveOpt : public IROptimize {
    public:

        void Optimize(IRBlock *block, OptResult *result) override;

    private:

        bool HasSideEffect(Instruction *instr, OptResult *result);

    };

}
