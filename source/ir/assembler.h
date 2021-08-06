//
// Created by SwiftGan on 2021/1/2.
//

#pragma once

#include "block.h"
#include <frontend/visitor.h>

namespace Svm::IR {

    class Assembler : public Decoder::Visitor {
    public:

        explicit Assembler(IRBlock &block) : block{block} {}

        void AdvancePC(u32 adv);

        constexpr void Terminal(const Direct &next) {
            Visitor::Terminal();
            block.Terminal(next);
        }

        constexpr void Terminal(const CheckCond &next) {
            Visitor::Terminal();
            block.Terminal(next);
        }

        constexpr void Terminal(const CheckBool &next) {
            Visitor::Terminal();
            block.Terminal(next);
        }

        constexpr void Terminal(const DeadEnd &next) {
            Visitor::Terminal();
            block.Terminal(next);
        }

        Label &Label();

        void BindLabel(IR::Label *label);
        
        void BranchCond(IR::Cond cond, IR::Label &label);
        
        void BranchBool(IR::Value &bool_value, IR::Label &label);

        IR::Value &Operand(const IR::Value &left, const IR::Value &right);

        IR::Value &Operand(const IR::Value &left, u32 ext);

        IR::Value &Operand(const IR::Value &left, const IR::Imm &imm, bool minus = false, u32 extend = 0);

        constexpr IRBlock &Block() {
            return block;
        }

#define INST0(name, ret) ret& name();
#define INST1(name, ret, arg1) ret& name(const arg1& a1);
#define INST2(name, ret, arg1, arg2) ret& name(const arg1& a1, const arg2& a2);
#define INST3(name, ret, arg1, arg2, arg3) ret& name(const arg1& a1, const arg2& a2, const arg3& a3);
#define INST4(name, ret, arg1, arg2, arg3, arg4) ret& name(const arg1& a1, const arg2& a2, const arg3& a3, const arg4& a4);

#include "instr_table.ir"

#undef INST0
#undef INST1
#undef INST2
#undef INST3
#undef INST4

    private:
        VAddr start;
        VAddr pc;
        IRBlock &block;
        List<IR::Label> labels;
    };

    class ConditionScope {
    public:

        explicit ConditionScope(Assembler *assembler, Cond cond);

        ~ConditionScope();

    private:
        Assembler *assembler;
        Label &next;
    };

}
