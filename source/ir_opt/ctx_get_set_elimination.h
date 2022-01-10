//
// Created by SwiftGan on 2021/1/4.
//

#pragma once

#include "ir_opt_result.h"
#include "optimize.h"

namespace Svm::IR {

    class CtxGetSetElimination : public IROptimize {
    public:

        void Optimize(IRBlock *block, OptResult *result) override;

    private:

        void FilterRegUseForOldVer(IRBlock *block, OptResult *result, OptHostReg *host_regs);

        bool ShouldSkip(Instruction* inst);

        void SetValue(Reg &reg, Instruction* def_inst) {
            cur_reg_value[reg.code] = def_inst;
        }

        void SetValue(VReg &reg, Instruction* def_inst) {
            cur_vreg_value[reg.code] = def_inst;
        }

        Instruction* GetValue(Reg &reg) {
            auto itr = cur_reg_value.find(reg.code);
            if (itr == cur_reg_value.end()) {
                return {};
            }
            return itr->second;
        }

        Instruction* GetValue(VReg &reg) {
            auto itr = cur_vreg_value.find(reg.code);
            if (itr == cur_vreg_value.end()) {
                return {};
            }
            return itr->second;
        }

        Map<Instruction*, Instruction*> host_regs_versions;
        Set<Instruction*> skip_instructions;
        Map<u8, Instruction*> cur_reg_value;
        Map<u8, Instruction*> cur_vreg_value;
    };

}
