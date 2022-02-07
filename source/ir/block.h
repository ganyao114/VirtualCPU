//
// Created by SwiftGan on 2021/1/2.
//

#pragma once

#include "instruction.h"
#include <base/lru_container.h>
#include <base/object_pool.h>
#include <base/slab_alloc.h>

namespace Svm::IR {

    struct Direct {
        Address next_block;
    };

    struct CheckCond {
        CheckCond() = default;
        CheckCond(Cond cond, Address then, Address else_) : cond(cond), then_(then),
                                                            else_(else_) {}
        Cond cond;
        Address then_;
        Address else_;
    };

    struct CheckBool {
        CheckBool() = default;
        CheckBool(Value bool_value, Address then, Address else_) : bool_value(bool_value), then_(then),
                                                         else_(else_) {}
        Value bool_value;
        Address then_;
        Address else_;
    };

    struct DeadEnd {

        enum Type {
            ILL_CODE,
            PAGE_FATAL
        };

        DeadEnd(Type type, const Imm &addr) : type(type), dead_addr(addr) {}

        Type type;
        Imm dead_addr;
    };

    class InstrContainer : CopyDisable {
    public:

        explicit InstrContainer() : instr_list{} {};

        explicit InstrContainer(ObjectPool<Instruction> *instr_pool) : instr_pool{instr_pool} {};

        explicit InstrContainer(SlabHeap<Instruction> *instr_slab) : instr_slab{instr_slab} {};

        constexpr Instruction *Create(OpCode op) {
            if (instr_pool) {
                return instr_pool->Create(op);
            }
            if (instr_slab) {
                auto res = instr_slab->New(op);
                if (res) {
                    return res;
                }
            }
            return &instr_list.emplace_back(op);
        }

        constexpr void Destroy(Instruction *instr) {
            if (instr_pool) {
                return;
            }
            if (instr_slab && instr_slab->Contains(reinterpret_cast<uintptr_t>(instr))) {
                return instr_slab->Delete(instr);
            }
        }

    private:
        SlabHeap<Instruction> *instr_slab{};
        ObjectPool<Instruction> *instr_pool{};
        List<Instruction> instr_list;
    };

    using InstrSequence = intrusive_list<Instruction, &Instruction::node>;

    constexpr IR::Cond FlipCond(IR::Cond cond) {
        switch (cond) {
            case IR::Cond::AT:
                return IR::Cond::BE;
            case IR::Cond::AE:
                return IR::Cond::BT;
            case IR::Cond::EQ:
                return IR::Cond::NE;
            case IR::Cond::NE:
                return IR::Cond::EQ;
            case IR::Cond::BT:
                return IR::Cond::AE;
            case IR::Cond::BE:
                return IR::Cond::AT;
            case IR::Cond::GT:
                return IR::Cond::LE;
            case IR::Cond::GE:
                return IR::Cond::LT;
            case IR::Cond::LT:
                return IR::Cond::GE;
            case IR::Cond::LE:
                return IR::Cond::GT;
            case IR::Cond::OF:
                return IR::Cond::NO;
            case IR::Cond::NO:
                return IR::Cond::OF;
            case IR::Cond::PA:
                return IR::Cond::NP;
            case IR::Cond::NP:
                return IR::Cond::PA;
            case IR::Cond::SN:
                return IR::Cond::NS;
            case IR::Cond::NS:
                return IR::Cond::SN;
            default:
                UNREACHABLE();
                break;
        }
    }

    class IRBlock : public LruContainer<IRBlock>::Node, BaseObject, CopyDisable {
    public:

        explicit IRBlock();

        explicit IRBlock(VAddr pc);

        explicit IRBlock(VAddr pc, ObjectPool<Instruction> *pool);

        explicit IRBlock(VAddr pc, SlabHeap<Instruction> *heap);

        ~IRBlock();

        enum TerminalType : u8 {
            DEAD_END,
            CHECK_COND,
            CHECK_BOOL,
            DIRECT
        };

        enum TerminalReason : u8 {
            EXCEPTION,
            BRANCH,
            FUNC_CALL,
            RET,
            SPLIT,
            UKN_INSTR,
        };

        void AdvancePC(const IR::Imm &imm);

        void BindLabel(IR::Label *label);

        void LinkLabel(IR::Label *label, Instruction *instr);

        void Terminal();

        void Terminal(const Direct &next);

        void Terminal(const CheckCond &next);

        void Terminal(const CheckBool &next);

        void Terminal(const DeadEnd &dead);

        constexpr void MarkReturn() {
            terminal_reason = RET;
        }

        constexpr void MarkFunctionCall() {
            terminal_reason = FUNC_CALL;
        }

        constexpr void MarkException() {
            terminal_reason = EXCEPTION;
        }

        constexpr TerminalReason GetTermReason() {
            return terminal_reason;
        }

        constexpr TerminalType GetTermType() {
            return terminal_type;
        }

        constexpr Direct &TermDirect() {
            return direct_terminal;
        }

        constexpr CheckBool &TermCheckBool() {
            return check_bool;
        }

        constexpr CheckCond &TermCheckCond() {
            return check_cond;
        }

        constexpr DeadEnd &TermDeadEnd() {
            return dead_end;
        }

        template <typename Ret, bool back = true>
        Ret &Emit(OpCode opcode, const std::initializer_list<Operand> &args) {
            Instruction *instr_ptr = instructions->Create(opcode);
            instr_ptr->InitRet<Ret>();
            instr_ptr->SetId(instr_sequence.size());
            std::for_each(args.begin(), args.end(), [instr_ptr, index = size_t(0)](const auto& arg) mutable {
                instr_ptr->SetParam(index, arg);
                index++;
            });
            if constexpr (back) {
                instr_sequence.push_back(*instr_ptr);
            } else {
                instr_sequence.push_front(*instr_ptr);
            }
            DefaultRetSize(instr_ptr);
            return instr_ptr->GetReturn().Get<Ret>();
        }

        void RemoveInstr(Instruction *instr);

        void IndexInstructions();

        void ClearIndexes();

        Instruction &Instr(u32 index);

        constexpr InstrSequence &Sequence() {
            return instr_sequence;
        }

        constexpr SharedPtr<InstrContainer> &Instructions() {
            return instructions;
        }

        constexpr VAddr StartPC() const {
            return start_pc;
        }

        constexpr u32 BlockSize() const {
            return current_offset;
        }
        
        constexpr bool Overlap(VAddr pc) const {
            return pc >= start_pc && pc < (start_pc + current_offset);
        }

        constexpr SharedMutex &Lock() {
            return lock;
        }

        bool Split(IRBlock *new_block, u32 offset);

        void EndBlock();

        void PrepareOpt();

        List<VAddr> NextBlocksAddress(bool include_call = false);

    private:

        void DefaultRetSize(Instruction *instr_ptr);

        void BuildUses();

        void ClearUses();

        void LogicalFlags();

        VAddr start_pc{};
        u32 current_offset{0};
        SharedPtr<InstrContainer> instructions;
        UnorderedMap<IR::Label*, Set<Instruction*>> pending_labels;
        Vector<Instruction*> instr_indexes;
        InstrSequence instr_sequence;
        Instruction terminal_instr{TERMINAL_INSTR_ID, OpCode::Terminal};
        union {
            Direct direct_terminal;
            CheckCond check_cond;
            CheckBool check_bool;
            DeadEnd dead_end;
        };
        TerminalType terminal_type;
        TerminalReason terminal_reason{BRANCH};
        SharedMutex lock{};
    };

}
