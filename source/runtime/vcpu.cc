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
        context->Helper().halt_flag = true;
    }

    bool VCpu::HasInterrupt() {
        return context->Helper().exception.action.reason;
    }

    void VCpu::ClearInterrupt() {
        context->Helper().exception.action = Exception::Action{Exception::NONE};
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

    void VCpu::CallSvc(u32 num) {
        context->Helper().exception.action = Exception::Action{Exception::SVC};
        context->Helper().exception.data = num;
    }

    void VCpu::CallHlt(u32 num) {
        context->Helper().exception.action = Exception::Action{Exception::HLT};
        context->Helper().exception.data = num;
    }

    void VCpu::CallBrk(u32 num) {
        context->Helper().exception.action = Exception::Action{Exception::BRK};
        context->Helper().exception.data = num;
        abort();
    }

    void VCpu::PageFatal(VAddr va, u8 flags) {
        context->Helper().exception.action = Exception::Action{Exception::BRK, flags};
        context->Helper().exception.data = va;
    }

    void VCpu::IllegalCode(VAddr va) {
        context->Helper().exception.action = Exception::Action{Exception::ILL_CODE};
        context->Helper().exception.data = va;
    }

    void VCpu::Yield() {
        context->Helper().exception.action = Exception::Action{Exception::YIELD};
    }
}


