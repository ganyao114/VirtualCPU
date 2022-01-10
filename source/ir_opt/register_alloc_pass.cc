//
// Created by swift on 1/9/21.
//

#include "register_alloc_pass.h"

namespace Svm::IR {

    void RegAllocatePass::Optimize(IRBlock *block, OptResult *result) {
        auto pass = result->GetRegAllocPass();

        for (auto &instr : block->Sequence()) {
            if (instr.Enabled()) {
                auto &ret = instr.GetReturn();
                if (ret.IsValue()) {
                    auto &ret_value = ret.Get<Value>();
                    if (ret_value.IsFloat()) {
                        pass->DefineFloatValue(ret_value);
                    } else {
                        pass->DefineValue(ret_value);
                    }
                }
                for (int i = 0; i < MAX_OPERANDS; ++i) {
                    auto &op = instr.GetOperand(i);
                    if (op.IsValue()) {
                        auto &param = op.Get<Value>();
                        if (param.IsFloat()) {
                            pass->UseFloatValue(&instr, param);
                        } else {
                            pass->UseValue(&instr, param);
                        }
                    } else if (op.IsAddress()) {
                        auto &param = op.Get<Address>();
                        if (!param.IsConst()) {
                            pass->UseValue(&instr, param.ValueAddress());
                        }
                    }
                }
            }
        }

        pass->AllocateForBlock();
    }
}