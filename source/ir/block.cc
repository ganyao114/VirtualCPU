//
// Created by SwiftGan on 2021/1/2.
//

#include "block.h"

namespace Svm::IR {

    IRBlock::IRBlock() {}

    IRBlock::IRBlock(VAddr pc) : start_pc(pc) {
        instructions = MakeShared<InstrContainer>();
    }

    IRBlock::IRBlock(VAddr pc, ObjectPool<Instruction> *pool) : start_pc(pc) {
        instructions = MakeShared<InstrContainer>(pool);
    }

    IRBlock::IRBlock(VAddr pc, SlabHeap<Instruction> *heap) : start_pc(pc) {
        instructions = MakeShared<InstrContainer>(heap);
    }

    void IRBlock::AdvancePC(const IR::Imm &imm) {
        Emit<Void>(OpCode::AdvancePC, {imm});
        current_offset += imm.Value<u32>();
    }

    void IRBlock::BindLabel(IR::Label *label) {
        auto &instrs = pending_labels[label];
        for (auto instr : instrs) {
            for (u8 index = 0; index < MAX_OPERANDS; index++) {
                auto &op = instr->GetOperand(index);
                if (op.IsLabel()) {
                    op.Get<Label>().ref = &Sequence().back();
                    break;
                }
            }
        }
        pending_labels.erase(label);
    }

    void IRBlock::LinkLabel(IR::Label *label, Instruction *instr) {
        pending_labels[label].emplace(instr);
    }

    void IRBlock::Terminal() {
        terminal_type = DEAD_END;
    }

    void IRBlock::Terminal(const Direct &next) {
        direct_terminal = next;
        terminal_type = DIRECT;
    }

    void IRBlock::Terminal(const CheckCond &next) {
        check_cond = next;
        terminal_type = CHECK_COND;
    }

    void IRBlock::Terminal(const CheckBool &next) {
        check_bool = next;
        terminal_type = CHECK_BOOL;
    }

    void IRBlock::Terminal(const DeadEnd &dead) {
        dead_end = dead;
        terminal_type = DEAD_END;
    }

    Instruction &IRBlock::Instr(u32 id) {
        if (id == TERMINAL_INSTR_ID) {
            return terminal_instr;
        }
        return *instr_indexes[id];
    }

    void IRBlock::RemoveInstr(Instruction *instr) {
        instr_sequence.erase(*instr);
        instructions->Destroy(instr);
    }

    bool IRBlock::Split(IRBlock *new_block, u32 offset) {
        auto itr = instr_sequence.begin();
        u32 cur_offset{0};
        for (; !itr.empty(); itr++) {
            if (cur_offset == offset) {
                break;
            }
            auto &instr = *itr;
            if (instr.GetOpCode() == OpCode::AdvancePC) {
                cur_offset += instr.GetParam<Imm>(0).Value<u32>();
            }
        }
        instr_sequence.split(new_block->instr_sequence, itr);
        new_block->instructions = instructions;
        new_block->start_pc = start_pc + offset;
        new_block->current_offset = current_offset - offset;
        new_block->terminal_type = terminal_type;
        new_block->terminal_reason = terminal_reason;
        current_offset = offset;
        switch (terminal_type) {
            case DIRECT:
                new_block->direct_terminal = direct_terminal;
                break;
            case CHECK_BOOL:
                new_block->check_bool = check_bool;
                break;
            case CHECK_COND:
                new_block->check_cond = check_cond;
                break;
            case DEAD_END:
                break;
        }
        Terminal(Direct{Imm(new_block->start_pc)});
        terminal_reason = SPLIT;
        return true;
    }

    void IRBlock::EndBlock() {
        LogicalFlags();
    }

    void IRBlock::PrepareOpt() {
        BuildUses();
    }

    void IRBlock::LogicalFlags() {
        for (auto &instr : instr_sequence) {
            switch (instr.GetOpCode()) {
                case OpCode::GetOverFlow:
                    instr.GetParam<Value>(0).Def()->SetCalAct({true});
                    break;
                default:
                    continue;
            }
        }
    }

    void IRBlock::BuildUses() {
        // uses
        for (auto &instr : instr_sequence) {
            instr.ClearUses();
            if (instr.GetOpCode() == OpCode::LoadImm) {
                instr.GetReturn().Get<Value>().SetSize(instr.GetParam<Imm>(0).GetSize());
            }
            for (u8 index = 0; index < MAX_OPERANDS; index++) {
                auto &op = instr.GetOperand(index);
                if (op.IsValue()) {
                    auto &value = op.Get<Value>();
                    if (value.Valid()) {
                        value.Def()->Use(&instr);
                    }
                } else if (op.IsAddress()) {
                    auto &address = op.Get<Address>();
                    if (!address.IsConst()) {
                        address.ValueAddress().Def()->Use(&instr);
                    }
                } else if (op.IsLabel()) {
                    auto label_ref = op.Get<Label>().Ref();
                    if (instr.GetOpCode() != OpCode::BindLabel && label_ref != &instr) {
                        label_ref->Use(&instr);
                    }
                }
            }
        }

        if (terminal_type == CHECK_BOOL) {
            auto instr = check_bool.bool_value.Def();
            instr->Use(&terminal_instr);
        }

        switch (terminal_type) {
            case DIRECT:
                if (!direct_terminal.next_block.IsConst()) {
                    auto instr = direct_terminal.next_block.ValueAddress().Def();
                    instr->Use(&terminal_instr);
                }
                break;
            case CHECK_BOOL:
                if (!check_bool.then_.IsConst()) {
                    auto instr = check_bool.then_.ValueAddress().Def();
                    instr->Use(&terminal_instr);
                }
                if (!check_bool.else_.IsConst()) {
                    auto instr = check_bool.else_.ValueAddress().Def();
                    instr->Use(&terminal_instr);
                }
                break;
            case CHECK_COND:
                if (!check_cond.then_.IsConst()) {
                    auto instr = check_cond.then_.ValueAddress().Def();
                    instr->Use(&terminal_instr);
                }
                if (!check_cond.else_.IsConst()) {
                    auto instr = check_cond.else_.ValueAddress().Def();
                    instr->Use(&terminal_instr);
                }
                break;
            case DEAD_END:
                break;
        }

    }

    void IRBlock::ClearUses() {
        for (auto &instr : instr_sequence) {
            instr.ClearUses();
        }
    }

    void IRBlock::DefaultRetSize(Instruction *instr) {
        IR::Size size{VOID};
        switch (instr->GetOpCode()) {
            case OpCode::SubValue:
            case OpCode::AddValue:
            case OpCode::AddImm:
            case OpCode::SubImm:
            case OpCode::AndValue:
            case OpCode::AndImm:
            case OpCode::SbbImm:
            case OpCode::SbbValue:
            case OpCode::AdcImm:
            case OpCode::AdcValue:
            case OpCode::OrValue:
            case OpCode::OrImm:
            case OpCode::Asr:
            case OpCode::Rr:
            case OpCode::MulImm:
            case OpCode::MulValue:
            case OpCode::DivImm:
            case OpCode::DivValue:
            case OpCode::Lsr:
            case OpCode::Lsl:
            case OpCode::BitExtract:
            case OpCode::BitInsert:
            case OpCode::BitClear:
                size = instr->GetParam<Value>(0).GetSize();
                break;
            case OpCode::LoadImm:
                size = instr->GetParam<Imm>(0).GetSize();
                break;
            case OpCode::CheckCond:
            case OpCode::CheckFlag:
            case OpCode::GetFlag:
            case OpCode::GetCarry:
            case OpCode::GetOverFlow:
            case OpCode::GetSigned:
            case OpCode::GetZero:
            case OpCode::TestBit:
                size = U8;
                break;
            case OpCode::GetReg:
                size = instr->GetParam<Reg>(0).GetSize();
                break;
            case OpCode::GetVReg:
                size = instr->GetParam<VReg>(0).GetSize();
                break;
            case OpCode::ZeroExtend:
                size = instr->GetParam<Size>(1);
                break;
            default:
                break;
        }
        if (size != VOID) {
            instr->GetReturn().Get<Value>().SetSize(size);
        }
    }

    void IRBlock::IndexInstructions() {
        instr_indexes.resize(Sequence().size());
        int cur_index{0};
        for (auto &instr : Sequence()) {
            instr.SetId(cur_index);
            instr_indexes[cur_index] = &instr;
            cur_index++;
        }
    }

    void IRBlock::ClearIndexes() {
        instr_indexes.clear();
        instr_indexes.resize(0);
    }

    IRBlock::~IRBlock() {
        while (!instr_sequence.empty()) {
            auto itr = instr_sequence.begin();
            instr_sequence.erase(itr);
            instructions->Destroy(&*itr);
        }
    }

    List<VAddr> IRBlock::NextBlocksAddress(bool include_call) {
        List<VAddr> blocks{};
        switch (GetTermType()) {
            case IR::IRBlock::DIRECT: {
                auto &address = TermDirect().next_block;
                if (address.IsConst()) {
                    if (include_call || (terminal_reason != FUNC_CALL)) {
                        blocks.emplace_back(address.ConstAddress().Value<VAddr>());
                    }
                }
                break;
            }
            case IR::IRBlock::CHECK_COND: {
                auto &address_then = TermCheckCond().then_;
                auto &address_else = TermCheckCond().else_;
                if (address_then.IsConst()) {
                    blocks.emplace_back(address_then.ConstAddress().Value<VAddr>());
                }
                if (address_else.IsConst()) {
                    blocks.emplace_back(address_else.ConstAddress().Value<VAddr>());
                }
                break;
            }
            case IR::IRBlock::CHECK_BOOL: {
                auto &address_then = TermCheckBool().then_;
                auto &address_else = TermCheckBool().else_;
                if (address_then.IsConst()) {
                    blocks.emplace_back(address_then.ConstAddress().Value<VAddr>());
                }
                if (address_else.IsConst()) {
                    blocks.emplace_back(address_else.ConstAddress().Value<VAddr>());
                }
                break;
            }
            case IR::IRBlock::DEAD_END:
                break;
        }
        return std::move(blocks);
    }
}
