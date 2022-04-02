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
#include "flags_set_elimination.h"
#include "instruction_id_pass.h"
#include "instruction_id_clear.h"

namespace Svm::IR {

    void OptimizeIR(IRBlock *block, OptResult *result) {
        std::vector<std::shared_ptr<IROptimize>> opts = {
                std::make_shared<ConstMemoryReadOpt>(),
                std::make_shared<ConstFoldingOpt>(),
                std::make_shared<ConstMemoryReadOpt>(),
                std::make_shared<InstrIdPass>(),
                std::make_shared<FlagsSetOpt>(),
                std::make_shared<DeadCodeRemoveOpt>(),
                std::make_shared<CtxGetSetElimination>(),
                std::make_shared<ValueExprFoldOpt>(),
                std::make_shared<InstrIdPass>(),
                std::make_shared<RegAllocatePass>(),
                std::make_shared<InstrIdClear>()
        };
        std::unique_lock guard(block->Lock());
        for (auto &opt : opts) {
            opt->Optimize(block, result);
        }
    }

}
