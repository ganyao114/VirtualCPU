//
// Created by SwiftGan on 2021/1/2.
//

#pragma once

#include <include/types.h>

namespace Svm::X86 {

    union RegW {
        u16 word;
        struct {
            u8 low;
            u8 high;
        };
    };

    union RegD {
        u32 dword;
        struct {
            RegW low;
            RegW high;
        };
    };

    union Reg {
        u64 qword;
        struct {
            RegD low;
            RegD high;
        };
    };

    union Xmm {
        double d[2];
        float f[4];

        u64 l[2];
        u32 i[4];
        u16 s[8];
        u8 b[16];
    };

    struct CpuFlags {
        // status flags
        bool cf; // carry: set to true when an arithmetic carry occurs
        bool pf; // parity: set to true if the number of bits set in the low 8 bits of the result is even
        bool af; // adjust: set to true if operation on least significant 4 bits caused carry
        bool zf; // zero: set if operation result is 0
        bool sf; // sign: set if most significant bit of result is 1
        bool of; // overflow: set when the result has a sign different from the expected one (carry into ^ carry out)
        bool df; // direction: controls increment/decrement of D register after string instructions
    };

    struct ThreadContext64 {
        union {
            Array<Reg, 16> regs;
            struct {
                Reg rax;
                Reg rcx;
                Reg rdx;
                Reg rbx;
                Reg rsp;
                Reg rbp;
                Reg rsi;
                Reg rdi;
                Reg r8;
                Reg r9;
                Reg r10;
                Reg r11;
                Reg r12;
                Reg r13;
                Reg r14;
                Reg r15;
            };
        };
        Reg es, cs, ss, ds, fs, gs;
        Reg pc;
        Array<Xmm, 16> xmms;
        CpuFlags flags;
    };

}
