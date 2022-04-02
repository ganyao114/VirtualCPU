//
// Created by 甘尧 on 2022/3/17.
//
#pragma once

#include "optimize.h"

namespace Svm::IR {

    class CtxPromotionPass : public IROptimize {
    public:
        void Optimize(IRBlock *block, OptResult *result) override;
    };

}
