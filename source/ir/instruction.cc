//
// Created by SwiftGan on 2021/1/2.
//

#include "instruction.h"

namespace Svm::IR {

    std::optional<u8> Instruction::GetIndex(Value &value) {
        for (u8 index = 0; index < MAX_OPERANDS; index++) {
            if (operands[index].IsValue()) {
                if (GetParam<Value>(index).Def() == value.Def()) {
                    return index;
                }
            }
        }
        return {};
    }

    void Instruction::Destroy() {
        for (auto use : uses) {
            use->UnUse(this);
        }
        uses.clear();
    }

}
