//
// Created by swift on 2021/6/12.
//

#include "interpreter.h"
#include <runtime/jit_runtime.h>
#include <backend/cpu.h>

namespace Svm::IR {

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
name(current->GetParam<IR::arg1>(0), current->GetParam<IR::arg2>(1), current->GetParam<IR::arg3>(2), current->GetParam<IR::arg4>(3));   \
break;

    Interpreter::Interpreter(JitRuntime *runtime, IRBlock *ir_block, VCpu *core, void *stack) : runtime(runtime), ir_block(ir_block),
                                                                                   core(core) {
        if (runtime->Guest64Bit()) {
            memory64 = &runtime->GetMemory64();
        } else {
            memory32 = &runtime->GetMemory32();
        }
        auto reg_size = ir_block->Instructions()->size();
        if (!stack) {
            stack = malloc(reg_size * sizeof(IRReg));
            interp_stack_mem = stack;
        }
        regs = {(IRReg *)stack, reg_size};

        switch (runtime->GuestArch()) {
            case CpuArch::X64:
                x64_ctx = core->Context()->X64();
                break;
            default:
                UNREACHABLE();
                break;
        }
    }

    Interpreter::~Interpreter() {
        if (interp_stack_mem) {
            free(interp_stack_mem);
        }
    }

    void Interpreter::Run() {
        auto cur_ir = ir_block->Sequence().begin();
        while (cur_ir != ir_block->Sequence().end()) {
            current = *cur_ir;

            // skip to label
            if (skip_to_id) {
                if (current->GetOpCode() != IR::OpCode::AdvancePC
                    && current->GetOpCode() != IR::OpCode::BindLabel) {
                    continue;
                }
            }

            switch (current->GetOpCode()) {
                case IR::OpCode::AdvancePC: {
                    block_offset += current->GetParam<IR::Imm>(0).Value<u32>();
                    // update pc
                    core->SetPC(ir_block->StartPC() + block_offset);
                    break;
                }
                case IR::OpCode::BindLabel: {
                    if (current->GetId() == skip_to_id) {
                        skip_to_id = {};
                    }
                    break;
                }
                case IR::OpCode::BranchCond: {
                    break;
                }
                case IR::OpCode::BranchBool: {
                    if (V(current->GetParam<Value>(0)).Get<bool>()) {
                        skip_to_id = current->GetParam<Label>(1).GetId();
                    }
                    break;
                }
#include <ir/instr_table.ir>
                default:
                    UNREACHABLE();
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

    void Interpreter::Fallback() {
    }

    void Interpreter::GetReg(IR::Reg &reg) {
        auto &ret_val = current->GetReturn().Get<Value>();
        std::memcpy(&V(ret_val).data, core->GeneralRegRef(reg.code), ret_val.SizeByte());
    }

    void Interpreter::GetVReg(IR::VReg &a1) {
        auto &ret_val = current->GetReturn().Get<Value>();
    }

    void Interpreter::SetReg(IR::Reg &reg, IR::Value &val) {
        std::memcpy(core->GeneralRegRef(reg.code), &V(val).data, val.SizeByte());
    }

    void Interpreter::SetVReg(IR::VReg &a1, IR::Value &a2) {

    }

    void Interpreter::GetZero(IR::Value &a1) {

    }

    void Interpreter::GetOverFlow(IR::Value &a1) {

    }

    void Interpreter::GetSigned(IR::Value &a1) {

    }

    void Interpreter::GetCarry(IR::Value &a1) {

    }

    void Interpreter::GetFlag(IR::Value &a1, IR::Flags &a2) {

    }

    void Interpreter::SetFlag(IR::Flags &a1, IR::Value &a2) {

    }

    void Interpreter::MarkFlag(IR::Flags &a1) {

    }

    void Interpreter::ClearFlags(IR::Flags &a1) {

    }

    void Interpreter::CheckCond(IR::Cond &cond) {
        auto &ret_val = current->GetReturn().Get<Value>();

        switch (runtime->GuestArch()) {
            case CpuArch::X64:
                Set(ret_val, CheckCondX64(cond));
                break;
            default:
                UNREACHABLE();
        }
    }

    void Interpreter::CheckFlag(IR::Flags &a1) {

    }

    void Interpreter::Yield() {
        core->MarkInterrupt();
        core->Yield();
    }

    void Interpreter::Nop() {

    }

    void Interpreter::Svc(IR::Imm &val) {
        core->MarkInterrupt();
        core->CallSvc(val.Value<u16>());
    }

    void Interpreter::Brk(IR::Imm &val) {
        core->MarkInterrupt();
        core->CallBrk(val.Value<u16>());
    }

    void Interpreter::Hlt(IR::Imm &val) {
        core->MarkInterrupt();
        core->CallHlt(val.Value<u16>());
    }

    void Interpreter::LoadImm(IR::Imm &imm) {
        auto &ret_val = current->GetReturn().Get<Value>();
        Set(ret_val, imm.data);
    }

    void Interpreter::PopRSB() {
        // IGNORE
    }

    void Interpreter::PushRSB(IR::Address &a1) {
        // IGNORE
    }

    void Interpreter::CheckHalt() {

    }

    void Interpreter::CompareAndSwap(IR::Address &addr, IR::Value &exp_val, IR::Value &new_val) {
        auto address = addr.IsConst() ? addr.ConstAddress().Value<u64>() : V(addr.ValueAddress()).Get<u64>();
        Operate([&](u64 expect, u64 the_new) {
            switch (exp_val.GetSize()) {
                case IR::Size::U8:
                    if (memory64) {
                        return memory64->AtomicCompareAndSwap<u8>(address, expect, the_new);
                    } else {
                        return memory32->AtomicCompareAndSwap<u8>(address, expect, the_new);
                    }
                case IR::Size::U16:
                    if (memory64) {
                        return memory64->AtomicCompareAndSwap<u16>(address, expect, the_new);
                    } else {
                        return memory32->AtomicCompareAndSwap<u16>(address, expect, the_new);
                    }
                case IR::Size::U32:
                    if (memory64) {
                        return memory64->AtomicCompareAndSwap<u32>(address, expect, the_new);
                    } else {
                        return memory32->AtomicCompareAndSwap<u32>(address, expect, the_new);
                    }
                case IR::Size::U64:
                    if (memory64) {
                        return memory64->AtomicCompareAndSwap<u64>(address, expect, the_new);
                    } else {
                        return memory32->AtomicCompareAndSwap<u64>(address, expect, the_new);
                    }
                default:
                    UNREACHABLE();
            }
        }, exp_val, new_val);
    }

    void Interpreter::WriteMemory(IR::Address &addr_val, IR::Value &val) {
        auto address = addr_val.IsConst() ? addr_val.ConstAddress().Value<u64>() : V(
                addr_val.ValueAddress()).Get<u64>();
        try {
            if (memory64) {
                memory64->WriteMemory(address, &V(val).data, val.SizeByte());
            } else {
                memory32->WriteMemory(address, &V(val).data, val.SizeByte());
            }
        } catch (MemoryException &e) {
            core->MarkInterrupt();
            core->PageFatal(e.addr, PageEntry::Write);
        }
    }

    void Interpreter::ReadMemory(IR::Address &addr_val) {
        auto &ret_val = current->GetReturn().Get<Value>();
        auto address = addr_val.IsConst() ? addr_val.ConstAddress().Value<u64>() : V(
                addr_val.ValueAddress()).Get<u64>();
        try {
            if (memory64) {
                memory64->ReadMemory(address, &V(ret_val).data, ret_val.SizeByte());
            } else {
                memory32->ReadMemory(address, &V(ret_val).data, ret_val.SizeByte());
            }
        } catch (MemoryException &e) {
            core->MarkInterrupt();
            core->PageFatal(e.addr, PageEntry::Read);
        }
    }

    void Interpreter::Lsl(IR::Value &left, IR::Imm &bits) {
        Operate([](u64 left, u64 right) {
            return left << right;
        }, left, bits);
    }

    void Interpreter::Lsr(IR::Value &left, IR::Imm &bits) {
        Operate([](u64 left, u64 right) {
            return left >> right;
        }, left, bits);
    }

    void Interpreter::AddImm(IR::Value &left, IR::Imm &right) {
        Operate([](u64 left, u64 right) {
            return left + right;
        }, left, right);
    }

    void Interpreter::AddValue(IR::Value &left, IR::Value &right) {
        Operate([this](u64 left, u64 right) {
            auto res = left + right;
            TestOverflow(current, left, right, res);
            return res;
        }, left, right);
    }

    void Interpreter::AdcImm(IR::Value &left, IR::Imm &right) {
        Operate([this](u64 left, u64 right) {
            auto res = left + right;
            TestOverflow(current, left, right, res);
            return res;
        }, left, right);
    }

    void Interpreter::AdcValue(IR::Value &left, IR::Value &right) {
        AddValue(left, right);
    }

    void Interpreter::SubImm(IR::Value &left, IR::Imm &right) {
        Operate([](u64 left, u64 right) {
            return left - right;
        }, left, right);
    }

    void Interpreter::SubValue(IR::Value &left, IR::Value &right) {
        Operate([](u64 left, u64 right) {
            return left - right;
        }, left, right);
    }

    void Interpreter::SbbImm(IR::Value &left, IR::Imm &right) {
        SubImm(left, right);
    }

    void Interpreter::SbbValue(IR::Value &left, IR::Value &right) {
        SubValue(left, right);
    }

    void Interpreter::BitInsert(IR::Value &a1, IR::Value &a2, IR::Imm &a3, IR::Imm &a4) {

    }

    void Interpreter::BitExtract(IR::Value &a1, IR::Imm &a2, IR::Imm &a3) {

    }

    void Interpreter::BitClear(IR::Value &value, IR::Imm &lsb, IR::Imm &bits) {
        auto res = Svm::BitClear(Get(value), lsb.Value<u8>(), bits.Value<u8>());
        auto &ret_val = current->GetReturn().Get<Value>();
        Set(ret_val, res);
    }

    void Interpreter::TestBit(IR::Value &a1, IR::Imm &a2) {

    }

    void Interpreter::ZeroExtend(IR::Value &value, IR::Size &size) {
        auto &ret_val = current->GetReturn().Get<Value>();
        Set(ret_val, Get(value));
    }

    void Interpreter::Rr(IR::Value &a1, IR::Imm &a2) {

    }

    void Interpreter::Asr(IR::Value &a1, IR::Imm &a2) {

    }

    void Interpreter::AndImm(IR::Value &left, IR::Imm &right) {
        Operate([](u64 left, u64 right) {
            return left + right;
        }, left, right);
    }

    void Interpreter::AndValue(IR::Value &left, IR::Value &right) {
        Operate([](u64 left, u64 right) {
            return left + right;
        }, left, right);
    }

    void Interpreter::OrImm(IR::Value &left, IR::Imm &right) {

    }

    void Interpreter::OrValue(IR::Value &left, IR::Value &right) {

    }

    void Interpreter::XorValue(IR::Value &a1, IR::Value &a2) {

    }

    void Interpreter::DivImm(IR::Value &a1, IR::Imm &a2) {

    }

    void Interpreter::MulImm(IR::Value &a1, IR::Imm &a2) {

    }

    void Interpreter::DivValue(IR::Value &a1, IR::Value &a2) {

    }

    void Interpreter::MulValue(IR::Value &a1, IR::Value &a2) {

    }

    void Interpreter::SignExtend(IR::Value &a1, IR::Size &a2) {

    }

    void Interpreter::Terminal() {
        switch (ir_block->GetTermType()) {
            case IR::IRBlock::DIRECT: {
                auto &address = ir_block->TermDirect().next_block;
                if (address.IsConst()) {
                    core->SetPC(address.ConstAddress().data);
                } else {
                    core->SetPC(V(address.ValueAddress()).Get<VAddr>());
                }
                break;
            }
            case IR::IRBlock::CHECK_COND: {
                auto &address_then = ir_block->TermCheckCond().then_;
                auto &address_else = ir_block->TermCheckCond().else_;
                if (CheckCondX64(ir_block->TermCheckCond().cond)) {
                    if (address_then.IsConst()) {
                        core->SetPC(address_then.ConstAddress().data);
                    } else {
                        core->SetPC(V(address_then.ValueAddress()).Get<VAddr>());
                    }
                } else {
                    if (address_else.IsConst()) {
                        core->SetPC(address_else.ConstAddress().data);
                    } else {
                        core->SetPC(V(address_else.ValueAddress()).Get<VAddr>());
                    }
                }
                break;
            }
            case IR::IRBlock::CHECK_BOOL: {
                auto &address_then = ir_block->TermCheckBool().then_;
                auto &address_else = ir_block->TermCheckBool().else_;
                auto &bool_val = ir_block->TermCheckBool().bool_value;
                if (V(bool_val).Get<bool>()) {
                    if (address_then.IsConst()) {
                        core->SetPC(address_then.ConstAddress().data);
                    } else {
                        core->SetPC(V(address_then.ValueAddress()).Get<VAddr>());
                    }
                } else {
                    if (address_else.IsConst()) {
                        core->SetPC(address_else.ConstAddress().data);
                    } else {
                        core->SetPC(V(address_else.ValueAddress()).Get<VAddr>());
                    }
                }
                break;
            }
            case IR::IRBlock::DEAD_END:
                core->MarkInterrupt();
                auto &term = ir_block->TermDeadEnd();
                if (term.type == IR::DeadEnd::ILL_CODE) {
                    core->IllegalCode(term.dead_addr.data);
                } else {
                    core->PageFatal(term.dead_addr.data, PageEntry::Exe);
                }
                break;
        }
    }

    // X86_64

    bool Interpreter::CheckCondX64(IR::Cond &cond) {
        bool result{false};
        switch (cond) {
            case Cond::EQ:
                result = x64_ctx->flags.zf;
                break;
            case Cond::NE:
                result = !x64_ctx->flags.sf;
                break;
            case Cond::CS:
                break;
            case Cond::CC:
                break;
            case Cond::MI:
                break;
            case Cond::PL:
                break;
            case Cond::VS:
                break;
            case Cond::VC:
                break;
            case Cond::HI:
                break;
            case Cond::LS:
                break;
            case Cond::GE:
                break;
            case Cond::LT:
                result = !x64_ctx->flags.sf;
                break;
            case Cond::GT:
                result = x64_ctx->flags.sf;
                break;
            case Cond::LE:
                break;
            case Cond::AL:
                break;
            case Cond::NV:
                break;
            case Cond::AT:
                break;
            case Cond::AE:
                break;
            case Cond::BT:
                break;
            case Cond::BE:
                break;
            case Cond::SN:
                break;
            case Cond::NS:
                break;
            case Cond::PA:
                result = x64_ctx->flags.sf;
                break;
            case Cond::NP:
                break;
        }
        return result;
    }
}