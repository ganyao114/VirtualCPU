//
// Created by SwiftGan on 2021/1/2.
//

#include "block.h"

namespace Svm::IR {

    IRBlock::IRBlock() {
        guest_offset_to_ir[0] = 0;
    }

    IRBlock::IRBlock(VAddr pc) : start_pc(pc) {
        guest_offset_to_ir[0] = 0;
        instructions = MakeShared<InstrContainer>();
    }

    void IRBlock::AdvancePC(const IR::Imm &imm) {
        Emit<Void>(OpCode::AdvancePC, {imm});
        current_offset += imm.Value<u32>();
        guest_offset_to_ir[current_offset] = instr_sequence.size();
    }

    void IRBlock::BindLabel(IR::Label *label) {
        for (auto id : pending_labels[label]) {
            auto &instr = Instr(id);
            for (u8 index = 0; index < MAX_OPERANDS; index++) {
                auto &op = instr.GetOperand(index);
                if (op.IsLabel()) {
                    op.Get<Label>().id = Sequence().back()->GetId();
                    break;
                }
            }
        }
        pending_labels.erase(label);
    }

    void IRBlock::LinkLabel(IR::Label *label, u32 id) {
        pending_labels[label].emplace(id);
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
        return (*instructions)[id];
    }

    void IRBlock::RemoveInstr(u32 id) {
        instructions->erase(id);
    }

    bool IRBlock::Split(const SharedPtr<IRBlock> &new_block, u32 offset) {
        auto itr = guest_offset_to_ir.find(offset);
        if (itr == guest_offset_to_ir.end()) {
            return false;
        }
        auto inst_index = itr->second;
        auto new_start = std::next(instr_sequence.begin(), inst_index);
        new_block->instructions = instructions;
        new_block->instr_sequence.splice(new_block->instr_sequence.begin(), instr_sequence, new_start, instr_sequence.end());
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
        for (auto &i : guest_offset_to_ir) {
            if (i.first >= offset) {
                new_block->guest_offset_to_ir[i.first - offset] = i.second - inst_index;
            }
        }
        Terminal(Direct{Imm(start_pc + offset)});
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
        for (auto instr : instr_sequence) {
            switch (instr->GetOpCode()) {
                case OpCode::GetOverFlow:
                    Instr(instr->GetParam<Value>(0).GetId()).SetCalAct({true});
                    break;
                default:
                    continue;
            }
        }
    }

    void IRBlock::BuildUses() {
        // uses
        for (auto instr : instr_sequence) {
            instr->ClearUses();
            if (instr->GetOpCode() == OpCode::LoadImm) {
                instr->GetReturn().Get<Value>().SetSize(instr->GetParam<Imm>(0).GetSize());
            }
            for (u8 index = 0; index < MAX_OPERANDS; index++) {
                auto &op = instr->GetOperand(index);
                if (op.IsValue()) {
                    auto &value = op.Get<Value>();
                    if (value.Valid()) {
                        Instr(value.GetId()).Use(instr->GetId());
                    }
                } else if (op.IsAddress()) {
                    auto &address = op.Get<Address>();
                    if (!address.IsConst()) {
                        Instr(address.ValueAddress().GetId()).Use(instr->GetId());
                    }
                } else if (op.IsLabel()) {
                    auto label_id = op.Get<Label>().GetId();
                    if (instr->GetOpCode() != OpCode::BindLabel && label_id != instr->GetId()) {
                        Instr(label_id).Use(instr->GetId());
                    }
                }
            }
        }

        if (terminal_type == CHECK_BOOL) {
            auto &instr = Instr(check_bool.bool_value.GetId());
            instr.Use(TERMINAL_INSTR_ID);
        }

        switch (terminal_type) {
            case DIRECT:
                if (!direct_terminal.next_block.IsConst()) {
                    auto &instr = Instr(direct_terminal.next_block.ValueAddress().GetId());
                    instr.Use(TERMINAL_INSTR_ID);
                }
                break;
            case CHECK_BOOL:
                if (!check_bool.then_.IsConst()) {
                    auto &instr = Instr(check_bool.then_.ValueAddress().GetId());
                    instr.Use(TERMINAL_INSTR_ID);
                }
                if (!check_bool.else_.IsConst()) {
                    auto &instr = Instr(check_bool.else_.ValueAddress().GetId());
                    instr.Use(TERMINAL_INSTR_ID);
                }
                break;
            case CHECK_COND:
                if (!check_cond.then_.IsConst()) {
                    auto &instr = Instr(check_cond.then_.ValueAddress().GetId());
                    instr.Use(TERMINAL_INSTR_ID);
                }
                if (!check_cond.else_.IsConst()) {
                    auto &instr = Instr(check_cond.else_.ValueAddress().GetId());
                    instr.Use(TERMINAL_INSTR_ID);
                }
                break;
            case DEAD_END:
                break;
        }

    }

    void IRBlock::ClearUses() {
        for (auto instr : instr_sequence) {
            instr->ClearUses();
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
        }
        if (size != VOID) {
            instr->GetReturn().Get<Value>().SetSize(size);
        }
    }
}
