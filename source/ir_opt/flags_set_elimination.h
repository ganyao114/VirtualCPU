//
// Created by swift on 2021/8/30.
//

#include "optimize.h"
#include "ir_opt_result.h"

namespace Svm::IR {

    class FlagsSetOpt : public IROptimize {
    public:

        void Optimize(IRBlock *block, OptResult *result) override;

    };

}
