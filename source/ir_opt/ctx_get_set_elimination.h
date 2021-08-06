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

        bool ShouldSkip(u32 id);

        void SetValue(Reg &reg, u32 def_inst) {
            cur_reg_value[reg.code] = def_inst;
        }

        void SetValue(VReg &reg, u32 def_inst) {
            cur_vreg_value[reg.code] = def_inst;
        }

        s32 GetValue(Reg &reg) {
            auto itr = cur_reg_value.find(reg.code);
            if (itr == cur_reg_value.end()) {
                return -1;
            }
            return itr->second;
        }

        s32 GetValue(VReg &reg) {
            auto itr = cur_vreg_value.find(reg.code);
            if (itr == cur_vreg_value.end()) {
                return -1;
            }
            return itr->second;
        }

        Map<u32, u32> host_regs_versions;
        Set<u32> skip_instructions;
        Map<u8, u32> cur_reg_value;
        Map<u8, u32> cur_vreg_value;
    };

}
