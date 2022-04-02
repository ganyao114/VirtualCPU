//
// Created by SwiftGan on 2021/1/3.
//

#pragma once

#include <ir/operand.h>
#include <ir/block.h>
#include <ir_opt/ir_opt_result.h>
#include <ir_opt/optimize.h>
#include "jit_context.h"

namespace Svm::A64 {

    class A64IROptResult;

    class IRCommitA64 : public IR::OptValueFold, public IR::OptFlagsSync {
    public:

        explicit IRCommitA64(IR::IRBlock *block, A64JitContext *context);

        void Translate();

        std::optional<Operand> GetOperand(IR::Value &value);

        std::optional<MemOperand> GetMemOperand(IR::Value &value);

        void Terminal();

        constexpr A64JitContext *Context() {
            return context;
        }

        constexpr IR::OptResult *IROptRes() {
            return opt_result.get();
        }

        bool CouldFold(IR::Instruction *dest, IR::Instruction *src) override;

        void MarkFold(IR::Instruction *value_src, std::set<IR::Instruction *> &dest_instr_set) override;

        Op *GetFoldOperand(IR::Instruction *value_src) override;

        bool CanSyncFlagSet(IR::Flags &flag, IR::Instruction *instr_flag_from) override;

        bool CanSyncFlagGet(IR::Flags &flag, IR::Instruction *instr_flag_consume) override;

        void SyncFlagSet(IR::Instruction *instr_flag_from, IR::Flags &flag) override;

        void SyncFlagGet(IR::Instruction *instr_flag_consume, IR::Flags &flag) override;

        void FlagsCanNotSync(IR::Flags &flag) override;

private:

        Label *GetLabel(u32 label_id);

        const Register &R(IR::Value &value);

        const VRegister &V(IR::Value &value);

        const Register &RTemp();

        const VRegister &VTemp();

        void Release(const Register &reg);

        void Release(const VRegister &reg);

#define INST0(name, ret) void name();
#define INST1(name, ret, arg1) void name(IR::arg1& a1);
#define INST2(name, ret, arg1, arg2) void name(IR::arg1& a1, IR::arg2& a2);
#define INST3(name, ret, arg1, arg2, arg3) void name(IR::arg1& a1, IR::arg2& a2, IR::arg3& a3);
#define INST4(name, ret, arg1, arg2, arg3, arg4) void name(IR::arg1& a1, IR::arg2& a2, IR::arg3& a3, IR::arg4& a4);

#include <ir/instr_table.ir>

#undef INST0
#undef INST1
#undef INST2
#undef INST3

        VAddr start_pc{};
        u32 block_offset{};
        IR::Instruction *current{};
        IR::IRBlock *block{};
        std::unique_ptr<IR::OptResult> opt_result{};
        A64JitContext *context{};
        std::map<u32, Label*> labels{};

        bool check_rsb_for_next{};
    };

    class A64IROptResult : public IR::OptResult {
    public:

        explicit A64IROptResult(IRCommitA64 *commit_a64);

        IR::OptHostReg *GetOptHostReg() override;

        IR::OptValueFold *GetOptValueFold() override;

        IR::OptConstRead *GetOptConstRead() override;

        IR::OptFlagsSync *GetOptFlagsGetSet() override;

        IR::RegAllocPass *GetRegAllocPass() override;

    private:
        IRCommitA64 *commit_a64{};
        std::unique_ptr<IR::OptConstRead> const_read{};
    };

}
