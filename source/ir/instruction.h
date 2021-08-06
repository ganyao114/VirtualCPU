//
// Created by SwiftGan on 2021/1/2.
//

#pragma once

#include "operand.h"

namespace Svm::IR {

    constexpr auto INVALID_INSTR_ID = 0;
    constexpr auto TERMINAL_INSTR_ID = 0xFFFFFFFF;
    constexpr auto MAX_OPERANDS = 4;

    enum class OpCode : u8 {
        UNK = 0,
        Terminal,
        AdvancePC,
        BindLabel,
        BranchCond,
        BranchBool,
#define INST0(x, ...) x,
#define INST1(x, ...) x,
#define INST2(x, ...) x,
#define INST3(x, ...) x,
#define INST4(x, ...) x,
#define Type(x)

#include "instr_table.ir"

#undef INST0
#undef INST1
#undef INST2
#undef INST3
#undef INST4
#undef Type
        COUNT
    };

    class Instruction : public BaseObject {
    public:

        explicit Instruction() : opcode(OpCode::UNK) {}

        explicit Instruction(OpCode opcode) : opcode(opcode) {}

        explicit Instruction(u32 id, OpCode opcode) : id(id), opcode(opcode) {}

        constexpr void SetOpCode(OpCode opcode) {
            this->opcode = opcode;
        }

        constexpr void SetId(u32 id) {
            this->id = id;
        }

        constexpr u32 GetId() {
            return id;
        }

        constexpr OpCode GetOpCode() {
            return opcode;
        }

        constexpr void SetParam(u8 index, const Operand &operand) {
            operands[index] = operand;
        }

        constexpr Operand &GetOperand(u8 index) {
            return operands[index];
        }

        template<typename T>
        constexpr T &GetParam(u8 index) {
            return operands[index].Get<T>();
        }
        
        Optional<u8> GetIndex(Value &value);

        constexpr void SetReturn(const Operand &operand) {
            ret = operand;
        }

        template<typename T>
        constexpr void SetReturn(u32 id) {
            ret = T{};
            if (ret.IsValue()) {
                auto &value = ret.Get<Value>();
                value.id = id;
                value.is_float = FloatResult();
            }
        }

        constexpr Operand &GetReturn() {
            return ret;
        }

        constexpr void Use(u32 id) {
            uses.emplace(id);
        }

        constexpr void UnUse(u32 id) {
            uses.erase(id);
        }

        constexpr Set<u32> &GetUses() {
            return uses;
        }

        constexpr void ClearUses() {
            uses.clear();
        }
        
        constexpr bool FloatResult() {
            switch (opcode) {
                case OpCode::GetVReg:
                    return true;
                default:
                    return false;
            }
        }

        constexpr void SetCalAct(const CalAct &act) {
            SetParam(3, act);
        }

        constexpr bool HasCalAct() {
            return operands[3].IsCalAct();
        }

        constexpr CalAct &GetCalAct() {
            return GetParam<CalAct>(3);
        }

    private:
        u32 id{INVALID_INSTR_ID};
        OpCode opcode;
        Array<Operand, MAX_OPERANDS> operands;
        Operand ret{};
        Set<u32> uses{};
    };

}
