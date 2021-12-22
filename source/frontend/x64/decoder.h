//
// Created by SwiftGan on 2021/1/1.
//

#pragma once

#include <frontend/decoder.h>
#include <ir/assembler.h>
#include <asm/x86/64/registers_x86_64.h>
#include <externals/distorm/decoder.h>
#include <externals/distorm/mnemonics.h>
#include <memory/memory_interface.h>

namespace Svm::Decoder {

    struct X86RegInfo {

        enum Index : u8 {
            Rax = 0,
            Rcx,
            Rdx,
            Rbx,
            Rsp,
            Rbp,
            Rsi,
            Rdi,
            R8,
            R9,
            R10,
            R11,
            R12,
            R13,
            R14,
            R15,
            ES,
            CS,
            SS,
            DS,
            FS,
            GS,
            Rip,
            Xmm0,
            Xmm1,
            Xmm2,
            Xmm3,
            Xmm4,
            Xmm5,
            Xmm6,
            Xmm7,
            Xmm8,
            Xmm9,
            Xmm10,
            Xmm11,
            Xmm12,
            Xmm13,
            Xmm14,
            Xmm15
        };

        u8 code;
        Index index;
        IR::Size size;
        bool high;
    };

    constexpr X86RegInfo x86_regs_table[] = {
            {_RegisterType::R_RAX,   X86RegInfo::Rax,   IR::Size::U64,  false},
            {_RegisterType::R_RCX,   X86RegInfo::Rcx,   IR::Size::U64,  false},
            {_RegisterType::R_RDX,   X86RegInfo::Rdx,   IR::Size::U64,  false},
            {_RegisterType::R_RBX,   X86RegInfo::Rbx,   IR::Size::U64,  false},
            {_RegisterType::R_RSP,   X86RegInfo::Rsp,   IR::Size::U64,  false},
            {_RegisterType::R_RBP,   X86RegInfo::Rbp,   IR::Size::U64,  false},
            {_RegisterType::R_RSI,   X86RegInfo::Rsi,   IR::Size::U64,  false},
            {_RegisterType::R_RDI,   X86RegInfo::Rdi,   IR::Size::U64,  false},
            {_RegisterType::R_R8,    X86RegInfo::R8,    IR::Size::U64,  false},
            {_RegisterType::R_R9,    X86RegInfo::R9,    IR::Size::U64,  false},
            {_RegisterType::R_R10,   X86RegInfo::R10,   IR::Size::U64,  false},
            {_RegisterType::R_R11,   X86RegInfo::R11,   IR::Size::U64,  false},
            {_RegisterType::R_R12,   X86RegInfo::R12,   IR::Size::U64,  false},
            {_RegisterType::R_R13,   X86RegInfo::R13,   IR::Size::U64,  false},
            {_RegisterType::R_R14,   X86RegInfo::R14,   IR::Size::U64,  false},
            {_RegisterType::R_R15,   X86RegInfo::R15,   IR::Size::U64,  false},
            {_RegisterType::R_EAX,   X86RegInfo::Rax,   IR::Size::U32,  false},
            {_RegisterType::R_ECX,   X86RegInfo::Rcx,   IR::Size::U32,  false},
            {_RegisterType::R_EDX,   X86RegInfo::Rdx,   IR::Size::U32,  false},
            {_RegisterType::R_EBX,   X86RegInfo::Rbx,   IR::Size::U32,  false},
            {_RegisterType::R_ESP,   X86RegInfo::Rsp,   IR::Size::U32,  false},
            {_RegisterType::R_EBP,   X86RegInfo::Rbp,   IR::Size::U32,  false},
            {_RegisterType::R_ESI,   X86RegInfo::Rsi,   IR::Size::U32,  false},
            {_RegisterType::R_EDI,   X86RegInfo::Rdi,   IR::Size::U32,  false},
            {_RegisterType::R_R8D,   X86RegInfo::R8,    IR::Size::U32,  false},
            {_RegisterType::R_R9D,   X86RegInfo::R9,    IR::Size::U32,  false},
            {_RegisterType::R_R10D,  X86RegInfo::R10,   IR::Size::U32,  false},
            {_RegisterType::R_R11D,  X86RegInfo::R11,   IR::Size::U32,  false},
            {_RegisterType::R_R12D,  X86RegInfo::R12,   IR::Size::U32,  false},
            {_RegisterType::R_R13D,  X86RegInfo::R13,   IR::Size::U32,  false},
            {_RegisterType::R_R14D,  X86RegInfo::R14,   IR::Size::U32,  false},
            {_RegisterType::R_R15D,  X86RegInfo::R15,   IR::Size::U32,  false},
            {_RegisterType::R_AX,    X86RegInfo::Rax,   IR::Size::U16,  false},
            {_RegisterType::R_CX,    X86RegInfo::Rcx,   IR::Size::U16,  false},
            {_RegisterType::R_DX,    X86RegInfo::Rdx,   IR::Size::U16,  false},
            {_RegisterType::R_BX,    X86RegInfo::Rbx,   IR::Size::U16,  false},
            {_RegisterType::R_SP,    X86RegInfo::Rsp,   IR::Size::U16,  false},
            {_RegisterType::R_BP,    X86RegInfo::Rbp,   IR::Size::U16,  false},
            {_RegisterType::R_SI,    X86RegInfo::Rsi,   IR::Size::U16,  false},
            {_RegisterType::R_DI,    X86RegInfo::Rdi,   IR::Size::U16,  false},
            {_RegisterType::R_R8W,   X86RegInfo::R8,    IR::Size::U16,  false},
            {_RegisterType::R_R9W,   X86RegInfo::R9,    IR::Size::U16,  false},
            {_RegisterType::R_R10W,  X86RegInfo::R10,   IR::Size::U16,  false},
            {_RegisterType::R_R11W,  X86RegInfo::R11,   IR::Size::U16,  false},
            {_RegisterType::R_R12W,  X86RegInfo::R12,   IR::Size::U16,  false},
            {_RegisterType::R_R13W,  X86RegInfo::R13,   IR::Size::U16,  false},
            {_RegisterType::R_R14W,  X86RegInfo::R14,   IR::Size::U16,  false},
            {_RegisterType::R_R15W,  X86RegInfo::R15,   IR::Size::U16,  false},
            {_RegisterType::R_AL,    X86RegInfo::Rax,   IR::Size::U8,   false},
            {_RegisterType::R_CL,    X86RegInfo::Rcx,   IR::Size::U8,   false},
            {_RegisterType::R_DL,    X86RegInfo::Rdx,   IR::Size::U8,   false},
            {_RegisterType::R_BL,    X86RegInfo::Rbx,   IR::Size::U8,   false},
            {_RegisterType::R_AH,    X86RegInfo::Rax,   IR::Size::U8,   true},
            {_RegisterType::R_CH,    X86RegInfo::Rcx,   IR::Size::U8,   true},
            {_RegisterType::R_DH,    X86RegInfo::Rdx,   IR::Size::U8,   true},
            {_RegisterType::R_BH,    X86RegInfo::Rbx,   IR::Size::U8,   true},
            {_RegisterType::R_R8B,   X86RegInfo::R8,    IR::Size::U8,   true},
            {_RegisterType::R_R9B,   X86RegInfo::R9,    IR::Size::U8,   true},
            {_RegisterType::R_R10B,  X86RegInfo::R10,   IR::Size::U8,   true},
            {_RegisterType::R_R11B,  X86RegInfo::R11,   IR::Size::U8,   true},
            {_RegisterType::R_R12B,  X86RegInfo::R12,   IR::Size::U8,   true},
            {_RegisterType::R_R13B,  X86RegInfo::R13,   IR::Size::U8,   true},
            {_RegisterType::R_R14B,  X86RegInfo::R14,   IR::Size::U8,   true},
            {_RegisterType::R_R15B,  X86RegInfo::R15,   IR::Size::U8,   true},
            {_RegisterType::R_SPL,   X86RegInfo::Rsp,   IR::Size::U8,   false},
            {_RegisterType::R_BPL,   X86RegInfo::Rbp,   IR::Size::U8,   false},
            {_RegisterType::R_SIL,   X86RegInfo::Rsi,   IR::Size::U8,   false},
            {_RegisterType::R_DIL,   X86RegInfo::Rdi,   IR::Size::U8,   false},
            {_RegisterType::R_ES,    X86RegInfo::ES,    IR::Size::U64,  false},
            {_RegisterType::R_CS,    X86RegInfo::CS,    IR::Size::U64,  false},
            {_RegisterType::R_SS,    X86RegInfo::SS,    IR::Size::U64,  false},
            {_RegisterType::R_DS,    X86RegInfo::DS,    IR::Size::U64,  false},
            {_RegisterType::R_FS,    X86RegInfo::FS,    IR::Size::U64,  false},
            {_RegisterType::R_GS,    X86RegInfo::GS,    IR::Size::U64,  false},
            {_RegisterType::R_RIP,   X86RegInfo::Rip,   IR::Size::U64,  false},
            {_RegisterType::R_ST0,   X86RegInfo::Xmm0,  IR::Size::U128, false},
            {_RegisterType::R_ST1,   X86RegInfo::Xmm1,  IR::Size::U128, false},
            {_RegisterType::R_ST2,   X86RegInfo::Xmm2,  IR::Size::U128, false},
            {_RegisterType::R_ST3,   X86RegInfo::Xmm3,  IR::Size::U128, false},
            {_RegisterType::R_ST4,   X86RegInfo::Xmm4,  IR::Size::U128, false},
            {_RegisterType::R_ST5,   X86RegInfo::Xmm5,  IR::Size::U128, false},
            {_RegisterType::R_ST6,   X86RegInfo::Xmm6,  IR::Size::U128, false},
            {_RegisterType::R_ST7,   X86RegInfo::Xmm7,  IR::Size::U128, false},
            {_RegisterType::R_MM0,   X86RegInfo::Xmm0,  IR::Size::U64,  false},
            {_RegisterType::R_MM1,   X86RegInfo::Xmm1,  IR::Size::U64,  false},
            {_RegisterType::R_MM2,   X86RegInfo::Xmm2,  IR::Size::U64,  false},
            {_RegisterType::R_MM3,   X86RegInfo::Xmm3,  IR::Size::U64,  false},
            {_RegisterType::R_MM4,   X86RegInfo::Xmm4,  IR::Size::U64,  false},
            {_RegisterType::R_MM5,   X86RegInfo::Xmm5,  IR::Size::U64,  false},
            {_RegisterType::R_MM6,   X86RegInfo::Xmm6,  IR::Size::U64,  false},
            {_RegisterType::R_MM7,   X86RegInfo::Xmm7,  IR::Size::U64,  false},
            {_RegisterType::R_XMM0,  X86RegInfo::Xmm0,  IR::Size::U128, false},
            {_RegisterType::R_XMM1,  X86RegInfo::Xmm1,  IR::Size::U128, false},
            {_RegisterType::R_XMM2,  X86RegInfo::Xmm2,  IR::Size::U128, false},
            {_RegisterType::R_XMM3,  X86RegInfo::Xmm3,  IR::Size::U128, false},
            {_RegisterType::R_XMM4,  X86RegInfo::Xmm4,  IR::Size::U128, false},
            {_RegisterType::R_XMM5,  X86RegInfo::Xmm5,  IR::Size::U128, false},
            {_RegisterType::R_XMM6,  X86RegInfo::Xmm6,  IR::Size::U128, false},
            {_RegisterType::R_XMM7,  X86RegInfo::Xmm7,  IR::Size::U128, false},
            {_RegisterType::R_XMM8,  X86RegInfo::Xmm8,  IR::Size::U128, false},
            {_RegisterType::R_XMM9,  X86RegInfo::Xmm9,  IR::Size::U128, false},
            {_RegisterType::R_XMM10, X86RegInfo::Xmm10, IR::Size::U128, false},
            {_RegisterType::R_XMM11, X86RegInfo::Xmm11, IR::Size::U128, false},
            {_RegisterType::R_XMM12, X86RegInfo::Xmm12, IR::Size::U128, false},
            {_RegisterType::R_XMM13, X86RegInfo::Xmm13, IR::Size::U128, false},
            {_RegisterType::R_XMM14, X86RegInfo::Xmm14, IR::Size::U128, false},
            {_RegisterType::R_XMM15, X86RegInfo::Xmm15, IR::Size::U128, false},

    };

    FORCE_INLINE IR::Reg ToReg(const X86RegInfo &info) {
        return IR::Reg{info.index, info.size, info.high};
    }

    FORCE_INLINE IR::VReg ToVReg(const X86RegInfo &info) {
        return IR::VReg{info.index, info.size, info.high};
    }

    class X64Decoder : public BaseDecoder {
    public:

        X64Decoder(VAddr start, IR::Assembler *visitor, Memory::MemoryInterface<VAddr> *memory);

        void Decode() override;

    private:

        enum CpuFlags : u32 {
            Carry = IR::Flags::Carry,
            Overflow = IR::Flags::Overflow,
            Signed = IR::Flags::Signed,
            Zero = IR::Flags::Zero,
            Parity = 1 << 4,
            FlagAF = 1 << 5,
            FlagAll = Carry | Overflow | Signed | Zero | Parity | FlagAF
        };

        enum SSEMCSREnables : u32 {
            IM = 1 << 7,
            DM = 1 << 8,
            ZM = 1 << 9,
            OM = 1 << 10,
            UM = 1 << 11,
            PM = 1 << 12
        };

        enum SSEMXCSRModes : u32 {
            FZ = 1 << 13,
            DAZ = 1 << 14,
            RN = 1 << 15
        };

        enum SSEMXCSRExceptions : u32 {
            PE = 1 << 16,
            UE = 1 << 17,
            OE = 1 << 18,
            ZE = 1 << 19,
            DE = 1 << 20,
            IE = 1 << 21
        };

        using X86Operand = std::variant<IR::Value, IR::Imm, IR::Address>;

        bool IsV(_RegisterType reg);

        IR::Value &R(_RegisterType reg);

        IR::Value &V(_RegisterType reg);

        void R(_RegisterType reg, IR::Value &value);

        void V(_RegisterType reg, IR::Value &value);

        IR::Size GetSize(u32 bits);

        IR::Value Src(_DInst &insn, _Operand &operand);

        IR::Address AddrSrc(_DInst &insn, _Operand &operand);

        void Dst(_DInst &insn, _Operand &operand, IR::Value &value);

        IR::Address GetAddress(_DInst &insn, _Operand &operand);

        IR::Value Pop(_RegisterType reg, IR::Size size);

        void Push(IR::Value &value);

        bool DecodeSwitch(_DInst &insn);

        void DecodeMov(_DInst &insn);

        void DecodeAddSub(_DInst &insn, bool sub, bool save_res = true);

        void DecodeCondJump(_DInst &insn, IR::Cond cond);

        void DecodeZeroCheckJump(_DInst &insn, _RegisterType reg);

        void DecodeAddSubWithCarry(_DInst &insn, bool sub);

        void DecodeIncAndDec(_DInst &insn, bool dec);

        void DecodeAnd(_DInst &insn);

        void DecodeLea(_DInst &insn);

        void DecodeMulDiv(_DInst &insn, bool div);

        void DecodeCondMov(_DInst &insn, IR::Cond cond);

        void DecodeOr(_DInst &insn);

        void DecodeXor(_DInst &insn);

        VAddr start;
        VAddr pc;
        IR::Assembler *visitor;
        Memory::MemoryInterface<VAddr> *memory;
    };

}
