//
// Created by 甘尧 on 2022/1/10.
//

#pragma once

#include "optimize.h"

namespace Svm::IR {

    class InstrIdPass : public IROptimize {
    public:

        void Optimize(IRBlock *block, OptResult *result) override;

    };

}
