//
// Created by SwiftGan on 2021/1/3.
//

#include "ir_translator.h"
#include <ir_opt/const_memory_read_pass.h>

#define __ context->Masm().

namespace Svm::A64 {

#define INST0(name, ret) case IR::OpCode::name: \
name();   \
break;
#define INST1(name, ret, arg1) case IR::OpCode::name: \
name(current->GetParam<IR::arg1>(0));   \
break;
#define INST2(name, ret, arg1, arg2) case IR::OpCode::name: \
name(current->GetParam<IR::arg1>(0), current->GetParam<IR::arg2>(1));   \
break;
#define INST3(name, ret, arg1, arg2, arg3) case IR::OpCode::name: \
name(current->GetParam<IR::arg1>(0), current->GetParam<IR::arg2>(1), current->GetParam<IR::arg3>(2));   \
break;
#define INST4(name, ret, arg1, arg2, arg3, arg4) case IR::OpCode::name: \
name(current->GetParam<IR::arg1>(0), current->GetParam<IR::arg2>(1), current->GetParam<IR::arg3>(2), current->GetParam<IR::arg4>(4));   \
break;

    IRCommitA64::IRCommitA64(IR::IRBlock *block, A64JitContext *context) : start_pc(block->StartPC()), block(block),
                                                                           context(context) {
        opt_result = MakeUnique<A64IROptResult>(this);
    }


    void IRCommitA64::Translate() {
        auto cur_ir = block->Sequence().begin();
        while (cur_ir != block->Sequence().end()) {
            current = *cur_ir;
            context->TickIR(current->GetId());
            if (opt_result->IsEnable(current->GetId())) {
                switch (current->GetOpCode()) {
                    case IR::OpCode::AdvancePC: {
                        block_offset += current->GetParam<IR::Imm>(0).Value<u32>();
                        context->TickPC(start_pc + block_offset);
                        break;
                    }
                    case IR::OpCode::BindLabel: {
                        auto &ir_label = current->GetParam<IR::Label>(0);
                        __ Bind(GetLabel(ir_label.id));
                        break;
                    }
                    case IR::OpCode::BranchCond:
                        break;
                    case IR::OpCode::BranchBool: {
                        auto reg_mng = &context->RegMng();
                        auto &bool_val = current->GetParam<IR::Value>(0);
                        auto &ir_label = current->GetParam<IR::Label>(1);
                        auto reg = reg_mng->GetValueRegister(bool_val);
                        __ Cbnz(reg, GetLabel(ir_label.id));
                        break;
                    }

#include <ir/instr_table.ir>

                    default:
                        break;
                }
            }
            cur_ir++;
        }

        Terminal();

    }

#undef INST0
#undef INST1
#undef INST2
#undef INST3
#undef INST4

    const Register &IRCommitA64::R(IR::Value &value) {
        return context->RegMng().GetValueRegister(value);
    }

    const VRegister &IRCommitA64::V(IR::Value &value) {
        return context->RegMng().GetValueVRegister(value);
    }

    const Register &IRCommitA64::RTemp() {
        return context->RegMng().AcquireTmpX();
    }

    const VRegister &IRCommitA64::VTemp() {
        return context->RegMng().AcquireTmpV();
    }

    void IRCommitA64::Release(const Register &reg) {
        return context->RegMng().ReleaseTmpX(reg);
    }

    void IRCommitA64::Release(const VRegister &reg) {
        return context->RegMng().ReleaseTmpV(reg);
    }

    void IRCommitA64::Fallback() {
        auto pc = start_pc + block_offset;
        Exception::Action action{Exception::FALLBACK};
        context->RaiseException(action, pc);
    }

    void IRCommitA64::PushRSB(IR::Address &va) {
        assert(va.IsConst());
        context->PushRSB(va.ConstAddress().Value<u64>());
    }

    void IRCommitA64::PopRSB() {
        check_rsb_for_next = true;
    }

    Optional<Operand> IRCommitA64::GetOperand(IR::Value &value) {
        return {};
    }

    Optional<MemOperand> IRCommitA64::GetMemOperand(IR::Value &value) {
        if (context->EnabledMMU()) {
            // if enable mmu, we cannot merge to memory operand
            return {};
        }

        auto expr_fold = opt_result->GetOptValueFold();
        auto op = expr_fold->GetFoldOperand(value.GetId());

        if (!op) {
            return {};
        }

        auto &left = op->left;
        auto &right = op->right;

        auto &left_reg = R(left);

        switch (op->op) {
            case IR::OptValueFold::Op::Add: {
                if (std::holds_alternative<IR::Imm>(right)) {
                    auto &imm = std::get<IR::Imm>(right);
                    if (IsUint<12>(imm.data)) {
                        return MemOperand(left_reg, imm.data);
                    } else {
                        return {};
                    }
                } else {
                    auto &right_reg = R(std::get<IR::Value>(right));
                    return MemOperand(left_reg, right_reg);
                }
            }
            case IR::OptValueFold::Op::Sub:
                if (std::holds_alternative<IR::Imm>(right)) {
                    auto &imm = std::get<IR::Imm>(right);
                    if (IsUint<12>(imm.data)) {
                        return MemOperand(left_reg, -imm.data);
                    } else {
                        return {};
                    }
                } else {
                    return {};
                }
            case IR::OptValueFold::Op::Mul:

                break;
            case IR::OptValueFold::Op::Div:
                break;
        }

        return {};
    }

    void IRCommitA64::Nop() {
        __ Nop();
    }

    void IRCommitA64::LoadImm(IR::Imm &imm) {
        auto &ret_val = current->GetReturn().Get<IR::Value>();
        auto &ret_reg = R(ret_val);
        __ Mov(ret_reg, imm.data);
    }

    void IRCommitA64::Terminal() {

        auto forward = [&](IR::Address &address) -> void {
            if (address.IsConst()) {
                context->Forward(address.ConstAddress().data);
            } else {
                auto &rt = R(address.ValueAddress());
                context->Forward(rt, check_rsb_for_next);
            }
        };

        switch (block->GetTermType()) {
            case IR::IRBlock::DEAD_END: {
                auto &dead_end = block->TermDeadEnd();
                if (dead_end.type == IR::DeadEnd::PAGE_FATAL) {
                    Exception::Action action{};
                    action.reason = Exception::PAGE_FATAL;
                    context->RaiseException(action, context->BlockEnd());
                } else {
                    Exception::Action action{};
                    action.reason = Exception::ILL_CODE;
                    context->RaiseException(action, context->BlockEnd());
                }
                break;
            }
            case IR::IRBlock::DIRECT : {
                auto &next_block = block->TermDirect().next_block;
                forward(next_block);
                break;
            }
            case IR::IRBlock::CHECK_BOOL : {
                auto &bool_value = block->TermCheckBool().bool_value;
                auto &then_block = block->TermCheckBool().then_;
                auto &else_block = block->TermCheckBool().else_;
                auto &bool_reg = R(bool_value);
                auto label_else = context->LabelAlloc().AllocLabel();
                __ Cbnz(bool_reg.W(), label_else);
                forward(then_block);
                __ Bind(label_else);
                forward(else_block);
                break;
            }
            case IR::IRBlock::CHECK_COND : {
                auto &cond = block->TermCheckCond().cond;
                auto &then_block = block->TermCheckCond().then_;
                auto &else_block = block->TermCheckCond().else_;
                auto label_else = context->LabelAlloc().AllocLabel();
                break;
            }
            default:
                break;
        }
    }

    void IRCommitA64::Flush() {
        context->EndBlock();
        auto runtime = context->Runtime();
        auto cache_size = __ GetBuffer()->GetSizeInBytes();
        auto[base, cache] = runtime->GetBlockCache(start_pc);
        CodeBuffer buffer{};
        if (cache->BoundModule()) {
            auto module = runtime->GetModule(cache->GetModule().module_id);
            buffer = module->AllocBuffer(cache->GetModule().block_id, cache_size);
        } else {
            buffer = runtime->GetCodeCachePool().Alloc(cache_size);
        }
        context->Flush(buffer);
        memcpy(buffer.rw_data, __ GetBuffer()->GetStartAddress<u8 *>(), cache_size);
        buffer.Flush();
        if (!cache->BoundModule()) {
            runtime->GetDispatcher().GetEntry(
                    cache->GetDispatchIndex()).value = reinterpret_cast<VAddr>(buffer.exec_data);
        }
    }

    Label *IRCommitA64::GetLabel(u32 label_id) {
        auto &res = labels[label_id];
        if (!res) {
            res = context->LabelAlloc().AllocLabel();
        }
        return res;
    }

    bool IRCommitA64::CouldFold(IR::Instruction *dest, IR::Instruction *src) {
        return false;
    }

    void IRCommitA64::MarkFold(u32 value_src_id, Set<u32> &dest_instr_set) {
        OptValueFold::MarkFold(value_src_id, dest_instr_set);
    }

    IR::OptValueFold::Op *IRCommitA64::GetFoldOperand(u32 value_src_id) {
        return OptValueFold::GetFoldOperand(value_src_id);
    }

    A64IROptResult::A64IROptResult(IRCommitA64 *commit_a64) : commit_a64(commit_a64) {
        auto runtime = commit_a64->Context()->Runtime();
        if (runtime->Guest64Bit()) {
            const_read = MakeUnique<IR::OptConstReadImpl>(&runtime->GetMemory64());
        } else {
            const_read = MakeUnique<IR::OptConstReadImpl>(&runtime->GetMemory32());
        }
    }

    IR::OptHostReg *A64IROptResult::GetOptHostReg() {
        return &commit_a64->Context()->RegMng();
    }

    IR::OptValueFold *A64IROptResult::GetOptValueFold() {
        return commit_a64;
    }

    IR::OptConstRead *A64IROptResult::GetOptConstRead() {
        return const_read.get();
    }

    IR::OptFlagsSync *A64IROptResult::GetOptFlagsGetSet() {
        return commit_a64;
    }

    IR::RegAllocPass *A64IROptResult::GetRegAllocPass() {
        return &commit_a64->Context()->RegMng();
    }
}

#undef __
