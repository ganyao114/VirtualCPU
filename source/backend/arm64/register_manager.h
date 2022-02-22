//
// Created by swift on 1/11/21.
//

#pragma once

#include <base/marco.h>
#include <externals/vixl/aarch64/operands-aarch64.h>
#include <externals/vixl/aarch64/macro-assembler-aarch64.h>
#include <ir/instruction.h>
#include <ir/block.h>
#include <ir_opt/ir_opt_result.h>
#include <frontend/x64/decoder.h>
#include <frontend/x64/cpu.h>
#include "constants.h"

namespace Svm::A64 {

    using namespace vixl::aarch64;

    using namespace Decoder;

    struct RegX64CastA64 {
        const X86RegInfo::Index x64;
        const Register &a64;
    };

    struct VRegX64CastA64 {
        X86RegInfo::Index x64;
        const VRegister &a64;
    };

    constexpr RegX64CastA64 x64_to_a64_regs_table[] = {
            {X86RegInfo::Index::Rax, x19},
            {X86RegInfo::Index::Rcx, x20},
            {X86RegInfo::Index::Rdx, x21},
            {X86RegInfo::Index::Rbx, x22},
            {X86RegInfo::Index::Rsp, x23},
            {X86RegInfo::Index::Rbp, x24},
            {X86RegInfo::Index::Rsi, x25},
            {X86RegInfo::Index::Rdi, x26},
            {X86RegInfo::Index::R8,  x27},
            {X86RegInfo::Index::R9,  x28},
            {X86RegInfo::Index::R10, x4},
            {X86RegInfo::Index::R11, x5},
            {X86RegInfo::Index::R12, x6},
            {X86RegInfo::Index::R13, x7},
            {X86RegInfo::Index::R14, x8},
    };

    constexpr VRegX64CastA64 x64_to_a64_vregs_table[] = {
            {X86RegInfo::Index::Xmm0,  v19},
            {X86RegInfo::Index::Xmm1,  v20},
            {X86RegInfo::Index::Xmm2,  v21},
            {X86RegInfo::Index::Xmm3,  v22},
            {X86RegInfo::Index::Xmm4,  v23},
            {X86RegInfo::Index::Xmm5,  v24},
            {X86RegInfo::Index::Xmm6,  v25},
            {X86RegInfo::Index::Xmm7,  v26},
            {X86RegInfo::Index::Xmm8,  v27},
            {X86RegInfo::Index::Xmm9,  v28},
            {X86RegInfo::Index::Xmm10, v29},
            {X86RegInfo::Index::Xmm11, v30},
            {X86RegInfo::Index::Xmm12, v31},
            {X86RegInfo::Index::Xmm13, v16},
            {X86RegInfo::Index::Xmm14, v17},
            {X86RegInfo::Index::Xmm15, v18}
    };

    const Array<const Register, 9> x64_guest_tmps = {
            x9,
            x10,
            x11,
            x12,
            x13,
            x14,
            x15,
            x17,
            x18
    };

    const Array<const VRegister, 12> x64_guest_vtmps = {
            v7,
            v8,
            v9,
            v10,
            v11,
            v12,
            v13,
            v14,
            v15,
            v16,
            v17,
            v18
    };

    class RegisterManager : public IR::OptHostReg, public IR::RegAllocPass, public IR::GuestRegInterface {
    public:

        explicit RegisterManager() {};

        void Initialize(IR::IRBlock *block, CpuArch guest);

        void TickIR(u32 id);

        const Register &GetValueRegister(IR::Value &value);

        const VRegister &GetValueVRegister(IR::Value &value);

        bool CastedToHostReg(const IR::Reg &reg);

        bool CastedToHostVReg(const IR::VReg &reg);

        const Register &ToHostRegister(IR::Reg &reg);

        const VRegister &ToHostVRegister(IR::VReg &reg);

        const Register &Context();

        const Register &PageTale();

        const Register &IP();

        const Register &Status();

        const Register &AcquireTmpX();

        const VRegister &AcquireTmpV();

        void ReleaseTmpX(const Register &r);

        void ReleaseTmpV(const VRegister &r);

        void DefineValue(IR::Value &value) override;

        void DefineFloatValue(IR::Value &value) override;

        void UseValue(IR::Instruction *instr, IR::Value &value) override;

        void UseFloatValue(IR::Instruction *instr, IR::Value &value) override;

        u32 OffsetOf(const IR::Reg &reg) override;

        u32 OffsetOf(const IR::VReg &reg) override;

        u32 PCOffset() override;

        bool DirectSetHostReg(IR::Instruction *instr) override;

        bool DirectGetHostReg(IR::Instruction *instr, u8 op_index) override;

        void MarkDirectSetHostReg(IR::Instruction *instr, IR::Reg &reg) override;

        void MarkDirectSetHostReg(IR::Instruction *instr, IR::VReg &reg) override;

        void MarkDirectGetHostReg(IR::Instruction *instr, IR::Reg &reg) override;

        void MarkDirectGetHostReg(IR::Instruction *instr, IR::VReg &reg) override;

    private:
        void AllocateForBlock() override;

        bool CastHostReg(IR::Reg &reg) override;

        bool CastHostReg(IR::VReg &reg) override;

        // ret index
        Optional<u8> AllocInRegList(Vector<Map<u16, u16>> &regs, u16 define, u16 end);

        const Register &ctx = REG_CTX;
        const Register &page_ptr = REG_PT;
        const Register &status = REG_STATUS;
        const Register &ip = REG_IP;

        u32 current_ir_id{0};
        CpuArch arch;
        IR::IRBlock *block{};
        Bitset<31> general_register_in_use;
        Bitset<31> vector_register_in_use;
        Vector<Map<u16, u16>> reg_live_interval;
        Vector<Map<u16, u16>> vreg_live_interval;
        Map<u32, const Register> registers_allocated;
        Map<u32, const VRegister> vregisters_allocated;
        Map<u32, Pair<u16, u16>> define_to_use_interval;
    };

}
