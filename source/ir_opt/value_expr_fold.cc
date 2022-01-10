//
// Created by swift on 1/7/21.
//

#include "value_expr_fold.h"


// IR SSA -> HOST Non SSA Instr
//
// X86 Guest:
// MOV R1, [R0+10]

// IR SSA:
// 1. GetReg:   TMP0 <- R0
// 2. AddImm:   TMP1 <- TMP0 + 10
// 3. ReadMem:  TMP3 <- [TMP1]
// 4. SetReg:   TMP3 -> R1

// ARM64 Host:
// LDR X1, [X0, #10] (X0 = R0, X1 = R1)

namespace Svm::IR {

    void ValueExprFoldOpt::Optimize(IRBlock *block, OptResult *result) {
        auto opt_value_fold = result->GetOptValueFold();
        for (auto &instr : block->Sequence()) {
            if (!instr.Enabled()) {
                continue;
            }
            auto &ret_value = instr.GetReturn();
            if (!ret_value.IsValue()) {
                continue;
            }
            if (instr.GetOpCode() == OpCode::GetReg || instr.GetOpCode() == OpCode::GetVReg) {
                continue;
            }
            auto &uses = instr.GetUses();
            // if more than one use, do not fold
            if (uses.size() > 1) {
                continue;
            }
            bool fold{false};
            for (auto instr_dest : uses) {
                if (instr_dest->GetOpCode() == OpCode::SetReg || instr_dest->GetOpCode() == OpCode::SetVReg) {
                    fold = false;
                    break;
                }
                if (opt_value_fold->CouldFold(instr_dest, &instr)) {
                    fold = true;
                } else {
                    fold = false;
                    break;
                }
            }
            if (fold) {
                opt_value_fold->MarkFold(&instr, uses);
                instr.Disable();
            }
        }
    }

}
