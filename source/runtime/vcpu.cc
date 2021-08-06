//
// Created by swift on 2021/6/29.
//

#include <include/svm_cpu.h>
#include <backend/cpu.h>
#include "jit_runtime.h"

namespace Svm {

    VCpu::VCpu(SvmInstance *instance) : instance(instance) {
        context = new CPUContext();
        arch = instance->Runtime().GuestArch();
        instance->Runtime().RegisterCore(this);
    }

    VCpu::~VCpu() {
        instance->Runtime().UnRegisterCore(this);
        delete context;
    }

    VAddr VCpu::GetPC() {
        switch (arch) {
            case CpuArch::X64:
                return context->X64()->pc.qword;
            default:
                UNREACHABLE();
        }
    }

    void VCpu::SetPC(VAddr pc) {
        switch (arch) {
            case CpuArch::X64:
                context->X64()->pc.qword = pc;
                break;
            default:
                UNREACHABLE();
        }
    }

    void VCpu::Run(u64 ticks) {
        context->help.ticks_max = context->help.ticks_now + ticks;
        instance->Runtime().Run(this);
    }

    void VCpu::Halt() {
        MarkInterrupt();
        context->Helper().halt_flag = true;
    }

    void VCpu::ClearInterrupt() {
        interrupt = false;
        context->Helper().halt_flag = false;
    }

    u8 *VCpu::GeneralRegRef(int index) {
        switch (arch) {
            case CpuArch::X64:
                return reinterpret_cast<u8 *>(&context->X64()->regs[index]);
            default:
                UNREACHABLE();
        }
    }

    u8 *VCpu::VectorRegRef(int index) {
        switch (arch) {
            case CpuArch::X64:
                return reinterpret_cast<u8 *>(&context->X64()->xmms[index]);
            default:
                UNREACHABLE();
        }
    }

}


