//
// Created by SwiftGan on 2021/1/1.
//

#include "cpu.h"
#include "decoder.h"
#include "jit_visitor.h"
#include <math.h>

namespace Svm::Decoder {
#define __ visitor->

    constexpr u8 TimesToShift(u32 times) {
        switch (times) {
            case 1:
                return 0;
            case 2:
                return 1;
            case 4:
                return 2;
            case 8:
                return 3;
            default:
                UNREACHABLE();
                break;
        }
    }

    X64Decoder::X64Decoder(VAddr start, IR::Assembler *visitor, Memory::MemoryInterface<VAddr> *memory) : start(start), visitor(visitor),
                                                                                                          memory(memory) {}

    void X64Decoder::Decode() {
        pc = start;
        while (!visitor->Termed()) {
            auto code_ptr = memory->Get<u8>(pc);
            if (!code_ptr) {
                __ Terminal(IR::DeadEnd{IR::DeadEnd::PAGE_FATAL, pc});
                break;
            }
            _DInst insn = DisDecode(code_ptr, 0x10, 1);
            if (insn.opcode == UINT16_MAX) {
                __ Terminal(IR::DeadEnd{IR::DeadEnd::ILL_CODE, pc});
                break;
            }
            pc += insn.size;
            if (!DecodeSwitch(insn)) {
                __ Fallback();
            }
            visitor->AdvancePC(insn.size);
        }
    }

    bool X64Decoder::DecodeSwitch(_DInst &insn) {
        switch (insn.opcode) {
            case I_NOP:
                __ Nop();
                break;
            case I_HLT:
                __ Hlt(u8(0));
                break;
            case I_INT_3:
                __ Brk(u8(0));
                break;
            case I_SYSCALL:
                __ Svc(u8(0));
                break;
            case I_CALL: {
                Push(__ LoadImm(pc));
                __ PushRSB(IR::Address(pc));
                DecodeCondJump(insn, IR::Cond::AL);
                __ Block().MarkFunctionCall();
                break;
            }
            case I_RET: {
                auto ret_addr = Pop(_RegisterType::R_RIP, IR::U64);
                __ PopRSB();
                __ Terminal(IR::Direct{ret_addr});
                __ Block().MarkReturn();
                break;
            }
            case I_RETF: {
                auto ret_addr = Pop(_RegisterType::R_RIP, IR::U64);
                __ PopRSB();
                __ Terminal(IR::Direct{ret_addr});
                __ Block().MarkReturn();
                break;
            }
            case I_LEAVE:
                R(_RegisterType::R_RSP, R(_RegisterType::R_RBP));
                Pop(_RegisterType::R_RBP, IR::U64);
                break;
            case I_LEA:
                DecodeLea(insn);
                break;
            case I_JMP:
                DecodeCondJump(insn, IR::Cond::AL);
                break;
            case I_JA:
                DecodeCondJump(insn, IR::Cond::AT);
                break;
            case I_JAE:
                DecodeCondJump(insn, IR::Cond::AE);
                break;
            case I_JB:
                DecodeCondJump(insn, IR::Cond::BT);
                break;
            case I_JBE:
                DecodeCondJump(insn, IR::Cond::BE);
                break;
            case I_JZ:
                DecodeCondJump(insn, IR::Cond::EQ);
                break;
            case I_JNZ:
                DecodeCondJump(insn, IR::Cond::NE);
                break;
            case I_JG:
                DecodeCondJump(insn, IR::Cond::GT);
                break;
            case I_JGE:
                DecodeCondJump(insn, IR::Cond::GE);
                break;
            case I_JL:
                DecodeCondJump(insn, IR::Cond::LT);
                break;
            case I_JLE:
                DecodeCondJump(insn, IR::Cond::LE);
                break;
            case I_JS:
                DecodeCondJump(insn, IR::Cond::SN);
                break;
            case I_JNS:
                DecodeCondJump(insn, IR::Cond::NS);
                break;
            case I_JP:
                DecodeCondJump(insn, IR::Cond::PA);
                break;
            case I_JO:
                DecodeCondJump(insn, IR::Cond::OF);
                break;
            case I_JNO:
                DecodeCondJump(insn, IR::Cond::NO);
                break;
            case I_JNP:
                DecodeCondJump(insn, IR::Cond::NP);
                break;
            case I_JECXZ:
                DecodeZeroCheckJump(insn, _RegisterType::R_ECX);
                break;
            case I_JRCXZ:
                DecodeZeroCheckJump(insn, _RegisterType::R_RCX);
                break;
            case I_MOV:
                DecodeMov(insn);
                break;
            case I_CMOVA:
                DecodeCondMov(insn, IR::Cond::AT);
                break;
            case I_CMOVAE:
                DecodeCondMov(insn, IR::Cond::AE);
                break;
            case I_CMOVB:
                DecodeCondMov(insn, IR::Cond::BT);
                break;
            case I_CMOVBE:
                DecodeCondMov(insn, IR::Cond::BE);
                break;
            case I_CMOVZ:
                DecodeCondMov(insn, IR::Cond::EQ);
                break;
            case I_CMOVG:
                DecodeCondMov(insn, IR::Cond::GT);
                break;
            case I_CMOVGE:
                DecodeCondMov(insn, IR::Cond::GE);
                break;
            case I_CMOVL:
                DecodeCondMov(insn, IR::Cond::LT);
                break;
            case I_CMOVLE:
                DecodeCondMov(insn, IR::Cond::LE);
                break;
            case I_CMOVNZ:
                DecodeCondMov(insn, IR::Cond::NE);
                break;
            case I_CMOVNO:
                DecodeCondMov(insn, IR::Cond::NO);
                break;
            case I_CMOVO:
                DecodeCondMov(insn, IR::Cond::OF);
                break;
            case I_CMOVP:
                DecodeCondMov(insn, IR::Cond::PA);
                break;
            case I_CMOVNP:
                DecodeCondMov(insn, IR::Cond::NP);
                break;
            case I_CMOVS:
                DecodeCondMov(insn, IR::Cond::SN);
                break;
            case I_CMOVNS:
                DecodeCondMov(insn, IR::Cond::NS);
                break;
            case I_ADD:
                DecodeAddSub(insn , false);
                break;
            case I_SUB:
                DecodeAddSub(insn , true);
                break;
            case I_CMP:
                DecodeAddSub(insn , true, false);
                break;
            case I_ADC:
                DecodeAddSubWithCarry(insn , false);
                break;
            case I_SBB:
                DecodeAddSubWithCarry(insn , true);
                break;
            case I_INC:
                DecodeIncAndDec(insn, false);
                break;
            case I_DEC:
                DecodeIncAndDec(insn, true);
                break;
            case I_MUL:
                DecodeMulDiv(insn, false);
                break;
            case I_DIV:
                DecodeMulDiv(insn, true);
                break;
            default:
                return false;
        }
        return true;
    }

    void X64Decoder::DecodeMov(_DInst &insn) {
        auto &op0 = insn.ops[0];
        auto &op1 = insn.ops[1];

        auto src = Src(insn, op1);
        Dst(insn, op0, src);
    }

    void X64Decoder::DecodeAddSub(_DInst &insn, bool sub, bool save_res) {
        auto &op0 = insn.ops[0];
        auto &op1 = insn.ops[1];
        
        auto left = Src(insn, op0);
        auto right = Src(insn, op1);
        
        auto &result = sub ? __ SubValue(left, right) : __ AddValue(left, right);
        
        __ SetFlag(IR::Flags{Carry}, __ GetCarry(result));
        __ SetFlag(IR::Flags{Overflow}, __ GetOverFlow(result));
        __ SetFlag(IR::Flags{Signed}, __ GetSigned(result));
        __ SetFlag(IR::Flags{Parity}, __ GetFlag(result, IR::Flags{Parity}));
        __ SetFlag(IR::Flags{Zero}, __ GetZero(result));
        
        if (save_res) {
            Dst(insn, op0, result);
        }
    }

    void X64Decoder::DecodeCondJump(_DInst &insn, IR::Cond cond) {
        auto &op0 = insn.ops[0];
        
        auto address = AddrSrc(insn, op0);

        if (cond == IR::Cond::AL) {
            __ Terminal(IR::Direct{address});
        } else {
            IR::CheckCond check_cond{cond, address, IR::Imm{pc + insn.size}};
            __ Terminal(check_cond);
        }
    }

    void X64Decoder::DecodeZeroCheckJump(_DInst &insn, _RegisterType reg) {
        auto &op0 = insn.ops[0];
        auto &value_check = R(reg);
        auto address = AddrSrc(insn, op0);
        IR::CheckBool check_bool{value_check, IR::Imm{pc + insn.size}, address};
        __ Terminal(check_bool);
    }

    void X64Decoder::DecodeAddSubWithCarry(_DInst &insn, bool sub) {
        auto &op0 = insn.ops[0];
        auto &op1 = insn.ops[1];
        
        auto left = Src(insn, op0);
        auto right = Src(insn, op1);
        
        auto &result = sub ? __ SbbValue(left, right) : __ AdcValue(left, right);
        
        __ SetFlag(IR::Flags{Carry}, __ GetCarry(result));
        __ SetFlag(IR::Flags{Overflow}, __ GetOverFlow(result));
        __ SetFlag(IR::Flags{Signed}, __ GetSigned(result));
        __ SetFlag(IR::Flags{Parity}, __ GetFlag(result, IR::Flags{Parity}));
        __ SetFlag(IR::Flags{Zero}, __ GetZero(result));
        
        Dst(insn, op0, result);
    }

    void X64Decoder::DecodeIncAndDec(_DInst &insn, bool dec) {
        auto &op0 = insn.ops[0];
        auto src = Src(insn, op0);
        auto &result = dec ? __ SubImm(src, (u8)1) : __ AddImm(src, (u8)1);
        
        __ SetFlag(IR::Flags{Overflow}, __ GetOverFlow(result));
        __ SetFlag(IR::Flags{Signed}, __ GetSigned(result));
        __ SetFlag(IR::Flags{Parity}, __ GetFlag(result, IR::Flags{Parity}));
        __ SetFlag(IR::Flags{Zero}, __ GetZero(result));
        
        Dst(insn, op0, result);
    }

    void X64Decoder::DecodeMulDiv(_DInst &insn, bool div) {
        auto &op0 = insn.ops[0];
        auto left = Src(insn, op0);
        auto right = R(div ? _RegisterType::R_RAX : _RegisterType::R_RAX);

        __ ClearFlags(IR::Flags{FlagAll});
    }

    void X64Decoder::DecodeLea(_DInst &insn) {
        auto &op0 = insn.ops[0];
        auto &op1 = insn.ops[1];
        
        auto address = GetAddress(insn, op1);
        auto &address_value = address.IsConst() ? __ LoadImm(address.ConstAddress()) : address.ValueAddress();
        Dst(insn, op0, address_value);
    }

    void X64Decoder::DecodeCondMov(_DInst &insn, IR::Cond cond) {
        IR::ConditionScope scope(visitor, cond);
        DecodeMov(insn);
    }

    void X64Decoder::DecodeAnd(_DInst &insn) {
        auto &label = __ Label();
        __ BindLabel(&label);
    }

    IR::Value &X64Decoder::R(_RegisterType reg) {
        assert(reg <= _RegisterType::R_XMM15);
        return __ GetReg(ToReg(x86_regs_table[reg]));
    }

    IR::Value &X64Decoder::V(_RegisterType reg) {
        assert(reg <= _RegisterType::R_XMM15);
        return __ GetVReg(ToVReg(x86_regs_table[reg]));
    }

    void X64Decoder::R(_RegisterType reg, IR::Value &value) {
        __ SetReg(ToReg(x86_regs_table[reg]), value);
    }

    void X64Decoder::V(_RegisterType reg, IR::Value &value) {
        __ SetVReg(ToVReg(x86_regs_table[reg]), value);
    }

    IR::Size X64Decoder::GetSize(u32 bits) {
        switch (bits) {
            case 0:
                return IR::VOID;
            case 8:
                return IR::U8;
            case 16:
                return IR::U16;
            case 32:
                return IR::U32;
            case 64:
                return IR::U64;
            case 128:
                return IR::U128;
        }
        UNREACHABLE();
    }

    IR::Value X64Decoder::Src(_DInst &insn, _Operand &operand) {
        IR::Value value{};
        auto size = GetSize(operand.size);
        switch (operand.type) {
            case O_PC:
                value = __ LoadImm(pc + insn.imm.qword);
                break;
            case O_REG:
                if (operand.index == R_RIP) {
                    value = __ LoadImm(pc + insn.imm.qword);
                } else if (IsV(static_cast<_RegisterType>(operand.index))) {
                    value = V(static_cast<_RegisterType>(operand.index));
                } else {
                    value = R(static_cast<_RegisterType>(operand.index));
                }
                break;
            case O_IMM:
                value = __ LoadImm(IR::Imm{insn.imm.qword, size});
                break;
            case O_SMEM:
            case O_MEM:
                auto address = GetAddress(insn, operand);
                value = __ ReadMemory(address).SetSize(size);
                break;
        }

        return value;
    }

    IR::Address X64Decoder::AddrSrc(_DInst &insn, _Operand &operand) {
        IR::Address value{};
        switch (operand.type) {
            case O_PC:
                value = IR::Imm(pc + insn.imm.qword);
                break;
            case O_REG:
                if (operand.index == R_RIP) {
                    value = IR::Imm(pc + insn.imm.qword);
                } else if (IsV(static_cast<_RegisterType>(operand.index))) {
                    value = V(static_cast<_RegisterType>(operand.index));
                } else {
                    value = R(static_cast<_RegisterType>(operand.index));
                }
                break;
            case O_IMM:
                value = IR::Imm{insn.imm.qword, GetSize(operand.size)};
                break;
            case O_SMEM:
            case O_MEM:
                auto address = GetAddress(insn, operand);
                value = __ ReadMemory(address).SetSize(IR::U64);
                break;
        }
        return value;
    }

    void X64Decoder::Dst(_DInst &insn, _Operand &operand, IR::Value &value) {
        switch (operand.type) {
            case O_REG:
                if (IsV(static_cast<_RegisterType>(operand.index))) {
                    V(static_cast<_RegisterType>(operand.index), value);
                } else {
                    R(static_cast<_RegisterType>(operand.index), value);
                }
                break;
            case O_SMEM:
            case O_MEM:
                auto address = GetAddress(insn, operand);
                __ WriteMemory(address, value);
                break;
        }
    }

    bool X64Decoder::IsV(_RegisterType reg) {
        return reg >= R_ST0;
    }

    IR::Address X64Decoder::GetAddress(_DInst &insn, _Operand &operand) {
        IR::Address value{};
        switch (operand.type) {
            case O_MEM: {
                u8 segment{};
                if (insn.segment == R_NONE) {
                    switch (insn.base) {
                        case R_BP:
                        case R_EBP:
                        case R_RBP:
                            segment = R_SS;
                            break;
                        case R_RIP:
                            segment = R_CS;
                            break;
                        default:
                            segment = R_NONE;
                    }
                }
                IR::Imm offset = insn.disp;
                if (insn.base <= R_DR7) {
                    auto &value_base = R(static_cast<_RegisterType>(insn.base));
                    auto &value_rn = R(static_cast<_RegisterType>(operand.index));
                    auto &value_base_index = insn.scale ? __ AddValue(value_base,
                                                          __ Lsl(value_rn, insn.scale))
                                                        : __ AddValue(value_base, value_rn);
                    value = insn.disp ? __ AddImm(value_base_index, offset) : value_base_index;
                } else {
                    auto &value_rn = R(static_cast<_RegisterType>(operand.index));
                    auto &value_base_index = insn.scale ? __ Lsl(value_rn, insn.scale) : value_rn;
                    value = insn.disp ? __ AddImm(value_base_index, offset) : value_base_index;
                }
                break;
            }
            case O_SMEM:
                if (operand.index == R_RIP) {
                    value = IR::Address(pc + insn.disp);
                } else {
                    auto &base_reg = R(static_cast<_RegisterType>(operand.index));
                    value = __ AddImm(base_reg, insn.disp);
                }
                break;
            default:
                UNREACHABLE();
        }
        if (!value.IsConst() && value.ValueAddress().GetSize() != IR::U64) {
            value = __ ZeroExtend(value.ValueAddress(), IR::U64);
        }
        return value;
    }

    IR::Value X64Decoder::Pop(_RegisterType reg, IR::Size size) {
        auto size_byte = std::pow(2, size - 1);
        auto sp = _RegisterType::R_RSP;
        IR::Address address = R(sp);
        auto &value = __ ReadMemory(address).SetSize(size);
        if (IsV(reg)) {
            V(reg, value);
        } else {
            R(reg, value);
        }
        R(sp, __ AddImm(R(sp), (u8)size_byte));
        return value;
    }

    void X64Decoder::Push(IR::Value &value) {
        auto size = value.GetSize();
        auto size_byte = std::pow(2, size - 1);
        auto sp = _RegisterType::R_RSP;
        IR::Address address = R(sp);
        __ WriteMemory(R(sp), value);
        R(sp, __ SubImm(R(sp), (u8)size_byte));
    }

}
