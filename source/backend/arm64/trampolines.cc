//
// Created by swift on 2021/5/18.
//

#include "backend/trampolines.h"
#include <include/svm_cpu.h>
#include <runtime/jit_runtime.h>

using namespace Svm;

CPUContext *__svm_interrupt_callback_stub(CPUContext *context) {
    auto &exception = context->help.exception;
    auto vcpu = reinterpret_cast<VCpu*>(context->help.context_ptr);
    vcpu->MarkInterrupt();
    switch (context->help.exception.action.reason) {
        case Exception::SVC:
            vcpu->CallSvc(exception.Data<u32>());
            break;
        case Exception::HLT:
            vcpu->CallHlt(exception.Data<u32>());
            break;
        case Exception::BRK:
            vcpu->CallBrk(exception.Data<u32>());
            break;
        case Exception::ILL_CODE:
            vcpu->IllegalCode(exception.Data<VAddr>());
            break;
        case Exception::PAGE_FATAL:
            vcpu->PageFatal(exception.Data<VAddr>(), exception.action.flag);
            break;
        case Exception::FALLBACK:
            vcpu->Fallback();
            break;
        case Exception::NONE:
            break;
        case Exception::YIELD:
            break;
    }

    if (!vcpu->IsInterFastReturn()) {
        auto &runtime = vcpu->Instance().Runtime();
        if (!vcpu->HasInterrupt()) {
            context->help.code_cache = runtime.FindAndJit(vcpu->GetPC(), false);
        } else {
            // will return host
            context->help.code_cache = runtime.ReturnHostTrampoline();
        }
    }

    return context;
}

CPUContext *__svm_call_host_callback_stub(CPUContext *context) {
    using Func0 = void *(*)();
    using Func1 = void *(*)(void*);
    using Func2 = void *(*)(void*, void*);
    using Func3 = void *(*)(void*, void*, void*);
    using Func4 = void *(*)(void*, void*, void*, void*);

    auto &host_call = context->help.host_call;

    switch (host_call.func.argc) {
        case 0: {
            auto func = reinterpret_cast<Func0>(host_call.callback_base + host_call.func.func_offset);
            host_call.ret_val = func();
            break;
        }
        case 1: {
            auto func = reinterpret_cast<Func1>(host_call.callback_base + host_call.func.func_offset);
            host_call.ret_val = func(host_call.argv[0]);
            break;
        }
        case 2: {
            auto func = reinterpret_cast<Func2>(host_call.callback_base + host_call.func.func_offset);
            host_call.ret_val = func(host_call.argv[0], host_call.argv[1]);
            break;
        }
        case 3: {
            auto func = reinterpret_cast<Func3>(host_call.callback_base + host_call.func.func_offset);
            host_call.ret_val = func(host_call.argv[0], host_call.argv[1], host_call.argv[3]);
            break;
        }
        case 4: {
            auto func = reinterpret_cast<Func4>(host_call.callback_base + host_call.func.func_offset);
            host_call.ret_val = func(host_call.argv[0], host_call.argv[1], host_call.argv[3], host_call.argv[4]);
            break;
        }
        default:
            UNREACHABLE();
    }
    return context;
}

CPUContext *__svm_cache_miss_callback(CPUContext *context) {
    auto vcpu = reinterpret_cast<VCpu*>(context->help.context_ptr);
    auto &runtime = vcpu->Instance().Runtime();
    context->help.code_cache = runtime.FindAndJit(vcpu->GetPC(), true);
    return context;
}

CPUContext *__svm_bind_cache_callback(CPUContext *context) {
    auto vcpu = reinterpret_cast<VCpu*>(context->help.context_ptr);
    auto &runtime = vcpu->Instance().Runtime();
    context->help.code_cache = runtime.FindAndJit(vcpu->GetPC());
    return context;
}