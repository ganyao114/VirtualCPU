//
// Created by SwiftGan on 2021/1/2.
//

#include "assembler.h"

namespace Svm::IR {

    // IR Assembler
#define INST0(name, ret) ret& Assembler::name() { \
    return block.Emit<ret>(OpCode::name, {}); \
}

#define INST1(name, ret, arg1) ret& Assembler::name(const arg1& a1) { \
    return block.Emit<ret>(OpCode::name, {a1}); \
}

#define INST2(name, ret, arg1, arg2) ret& Assembler::name(const arg1& a1, const arg2& a2) { \
    return block.Emit<ret>(OpCode::name, {a1, a2}); \
}

#define INST3(name, ret, arg1, arg2, arg3) ret& Assembler::name(const arg1& a1, const arg2& a2, const arg3& a3) { \
    return block.Emit<ret>(OpCode::name, {a1, a2, a3}); \
}

#define INST4(name, ret, arg1, arg2, arg3, arg4) ret& Assembler::name(const arg1& a1, const arg2& a2, const arg3& a3, const arg4& a4) { \
    return block.Emit<ret>(OpCode::name, {a1, a2, a3, a4}); \
}

#include "instr_table.ir"

#undef INST0
#undef INST1
#undef INST2
#undef INST3
#undef INST4

    void Assembler::AdvancePC(u32 adv) {
        pc += adv;
        block.AdvancePC(Imm{adv});
    }

    Label &Assembler::Label() {
        return labels.emplace_back();
    }

    void Assembler::BindLabel(IR::Label *label) {
        auto instr = block.EmitInstr<Void>(OpCode::BindLabel, {IR::Label{nullptr}});
        instr->GetParam<IR::Label>(0).ref = instr;
        block.BindLabel(label);
    }

    void Assembler::BranchCond(IR::Cond cond, IR::Label &label) {
        block.Emit<Void>(OpCode::BranchCond, {cond, label});
        block.LinkLabel(&label, &block.Sequence().back());
    }

    void Assembler::BranchBool(IR::Value &bool_value, IR::Label &label) {
        block.Emit<Void>(OpCode::BranchBool, {bool_value, label});
        block.LinkLabel(&label, &block.Sequence().back());
    }

    IR::Value &Assembler::Operand(const IR::Value &left, u32 ext) {
        auto &value = Lsl(left, ext);
        return Lsl(left, ext);
    }

    IR::Value &Assembler::Operand(const IR::Value &left, const IR::Value &right) {
        auto &value = AddValue(left, right);
        return AddValue(left, right);
    }

    IR::Value &Assembler::Operand(const IR::Value &left, const IR::Imm &imm, bool minus, u32 extend) {
        if (minus) {
            auto &value = Lsl(SubImm(left, imm), extend);
            return Lsl(SubImm(left, imm), extend);
        } else {
            auto &value = Lsl(AddImm(left, imm), extend);
            return Lsl(AddImm(left, imm), extend);
        }
    }

    ConditionScope::ConditionScope(Assembler *assembler, Cond cond) : assembler(assembler), next(assembler->Label()) {
        assembler->BranchCond(FlipCond(cond), next);
    }

    ConditionScope::~ConditionScope() {
        assembler->BindLabel(&next);
    }

}
