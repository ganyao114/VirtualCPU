//
// Created by swift on 2021/5/18.
//

#pragma once

#include "backend/cpu.h"

extern "C" {

    using namespace Svm;

    CPUContext *__svm_cache_miss_trampoline(CPUContext *context);

    CPUContext *__svm_bind_cache_trampoline(CPUContext *context);

    CPUContext *__svm_call_host_trampoline(CPUContext *context);

    CPUContext *__svm_code_run_entry_x64(CPUContext *context);

    CPUContext *__svm_interrupt_trampoline_x64(CPUContext *context);

    CPUContext *__svm_interrupt_callback_stub(CPUContext *context);

    CPUContext *__svm_call_host_callback_stub(CPUContext *context);

    CPUContext *__svm_cache_miss_callback(CPUContext *context);

    CPUContext *__svm_bind_cache_callback(CPUContext *context);

    void __svm_return_to_host();

}