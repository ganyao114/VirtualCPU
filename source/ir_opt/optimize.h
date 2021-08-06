//
// Created by SwiftGan on 2021/1/4.
//

#pragma once

#include <ir/block.h>
#include "ir_opt_result.h"

namespace Svm::IR {

    class IROptimize : public BaseObject, CopyDisable {
    public:

        virtual void Optimize(IRBlock *block, OptResult *result) = 0;

    };

}
