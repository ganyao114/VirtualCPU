//
// Created by SwiftGan on 2021/1/2.
//

#pragma once

#include "operand.h"
#include <base/intrusive_list.hpp>

namespace Svm::IR {

    constexpr auto INVALID_INSTR_ID = 0xFFFFFFFF;
    constexpr auto TERMINAL_INSTR_ID = UINT16_MAX - 1;
    constexpr auto DISABLED_INSTR_ID = 0xFFFFFFF7;
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

#include "instr_table.ir"

#undef INST0
#undef INST1
#undef INST2
#undef INST3
#undef INST4
        COUNT
    };

    class Instruction : CopyDisable {
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

        [[nodiscard]] constexpr u32 GetId() const {
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
        
        std::optional<u8> GetIndex(Value &value);

        constexpr void SetReturn(const Operand &operand) {
            ret = operand;
        }

        template<typename T>
        constexpr void InitRet() {
            ret = T{};
            if (ret.IsValue()) {
                auto &value = ret.Get<Value>();
                value.def = this;
                value.is_float = FloatResult();
            }
        }

        constexpr Operand &GetReturn() {
            return ret;
        }

        constexpr void Use(Instruction *instr) {
            uses.emplace(instr);
        }

        constexpr void UnUse(Instruction *instr) {
            uses.erase(instr);
        }

        constexpr std::set<Instruction*> &GetUses() {
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

        constexpr void Disable() {
            id = DISABLED_INSTR_ID;
        }

        [[nodiscard]] constexpr bool Enabled() const {
            return id != DISABLED_INSTR_ID;
        }

        void Destroy();

        intrusive_node node{};

    private:
        u32 id{INVALID_INSTR_ID};
        OpCode opcode;
        Array<Operand, MAX_OPERANDS> operands;
        Operand ret{};
        std::set<Instruction*> uses{};
    };

}
