//
// Created by swift on 1/8/21.
//

#include "ir_opt_manager.h"
#include "dead_code_remove.h"
#include "const_memory_read_pass.h"
#include "const_folding_pass.h"
#include "ctx_get_set_elimination.h"
#include "value_expr_fold.h"
#include "register_alloc_pass.h"

namespace Svm::IR {

    void OptimizeIR(IRBlock *block, OptResult *result) {
        Vector<SharedPtr<IROptimize>> opts = {
                MakeShared<ConstMemoryReadOpt>(),
                MakeShared<ConstFoldingOpt>(),
                MakeShared<ConstMemoryReadOpt>(),
                MakeShared<DeadCodeRemoveOpt>(),
                MakeShared<CtxGetSetElimination>(),
                MakeShared<ValueExprFoldOpt>(),
                MakeShared<RegAllocatePass>()
        };
        for (auto &opt : opts) {
            opt->Optimize(block, result);
        }
    }

}
