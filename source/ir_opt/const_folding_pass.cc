//
// Created by swift on 1/8/21.
//

#include "const_folding_pass.h"

namespace Svm::IR {


    void ConstFoldingOpt::Optimize(IRBlock *block, OptResult *result) {
        for (auto &instr : block->Sequence()) {

            if (!instr.Enabled()) {
                continue;
            }

            bool could_opt{false};
            switch (instr.GetOpCode()) {
                case OpCode::AddImm:
                case OpCode::AndImm:
                case OpCode::MulImm:
                case OpCode::AdcImm:
                case OpCode::SubImm:
                case OpCode::SbbImm:
                case OpCode::DivImm:
                case OpCode::OrImm: {
                    auto val_from = instr.GetParam<Value>(0).Def();
                    if (val_from->GetOpCode() == OpCode::LoadImm) {

                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

}