//
// Created by swift on 1/8/21.
//

#pragma once

#include "ir_opt_result.h"
#include "optimize.h"

namespace Svm::IR {

    class ConstFoldingOpt : public IROptimize {
    public:

        enum FoldFlag {

        };

        void Optimize(IRBlock *block, OptResult *result) override;

    private:

        template <typename ContExpr>
        bool FoldCommutative(IRBlock *block, Instruction *instr, ContExpr expr) {

            auto left_expr = instr->GetParam<Value>(0).Def();

            if (left_expr->GetOpCode() != OpCode::LoadImm) {
                return false;
            }

            u64 left;
            u64 right;

            if (instr->GetOperand(1).IsImm()) {
                right = instr->GetParam<Imm>(1).data;
            } else {
                auto right_expr = instr->GetParam<Value>(1).Def();
                if (right_expr->GetOpCode() != OpCode::LoadImm) {
                    return false;
                }
                right = right_expr->GetParam<Imm>(0).data;
                right_expr->UnUse(instr);
            }
            left = left_expr->GetParam<Imm>(0).data;

            left_expr->UnUse(instr);
            instr->SetOpCode(OpCode::LoadImm);
            auto result = expr(left, right);
            instr->SetParam(0, Imm{result});
            instr->SetParam(1, Void{});
            instr->SetParam(2, Void{});

            instr->SetParam(3, CalActRes{});

            return true;
        }

    };

}
