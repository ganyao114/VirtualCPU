#include "../constants.h"

#if defined(__APPLE__)
#define cdecl(s) _##s
#else
#define cdecl(s) s
#endif


.align  4
.globl cdecl(__svm_save_host_callee)
cdecl(__svm_save_host_callee):
    stp x19, x20, [sp, #-16]!
    stp x21, x22, [sp, #-16]!
    stp x23, x24, [sp, #-16]!
    stp x25, x26, [sp, #-16]!
    stp x27, x28, [sp, #-16]!

    stp q8, q9,   [sp, #-32]!
    stp q10, q11, [sp, #-32]!
    stp q12, q13, [sp, #-32]!
    stp q14, q15, [sp, #-32]!
    ret

.align  4
.globl cdecl(__svm_restore_host_callee)
cdecl(__svm_restore_host_callee):
    ldp q14, q15, [sp], 32
    ldp q12, q13, [sp], 32
    ldp q10, q11, [sp], 32
    ldp q8, q9,   [sp], 32

    ldp x27, x28, [sp], 16
    ldp x25, x26, [sp], 16
    ldp x23, x24, [sp], 16
    ldp x21, x22, [sp], 16
    ldp x19, x20, [sp], 16
    ret

.align  4
.globl cdecl(__svm_save_host_caller)
cdecl(__svm_save_host_caller):
    stp x1, x2,   [sp, #-16]!
    stp x3, x4,   [sp, #-16]!
    stp x5, x6,   [sp, #-16]!
    stp x7, x8,   [sp, #-16]!
    stp x9, x10,  [sp, #-16]!
    stp x11, x12, [sp, #-16]!
    stp x13, x14, [sp, #-16]!
    stp x15, x16, [sp, #-16]!
    stp x17, x18, [sp, #-16]!

    stp q0, q1,   [sp, #-32]!
    stp q2, q3,   [sp, #-32]!
    stp q4, q5,   [sp, #-32]!
    stp q6, q7,   [sp, #-32]!
    ret

.align  4
.globl cdecl(__svm_restore_host_caller)
cdecl(__svm_restore_host_caller):
    ldp q6, q7,   [sp], 32
    ldp q4, q5,   [sp], 32
    ldp q2, q3,   [sp], 32
    ldp q0, q1,   [sp], 32

    ldp x17, x18, [sp], 16
    ldp x15, x16, [sp], 16
    ldp x13, x14, [sp], 16
    ldp x11, x12, [sp], 16
    ldp x9, x10,  [sp], 16
    ldp x7, x8,   [sp], 16
    ldp x5, x6,   [sp], 16
    ldp x3, x4,   [sp], 16
    ldp x1, x2,   [sp], 16
    ret

.align  4
.globl cdecl(__svm_save_guest_register_x64)
cdecl(__svm_save_guest_register_x64):
    str REG_STATUS, [REG_CTX, OFFSET_X64_STATUS]
    mov REG_IP, REG_CTX
    stp x19, x20, [REG_IP], #16
    stp x21, x22, [REG_IP], #16
    stp x23, x24, [REG_IP], #16
    stp x25, x26, [REG_IP], #16
    stp x27, x28, [REG_IP], #16
    stp x4, x5,   [REG_IP], #16
    stp x6, x7,   [REG_IP], #16
    str x8,       [REG_IP], #8
    add REG_IP, REG_CTX, #OFFSET_X64_VREGS
    stp q19, q20, [REG_IP], #32
    stp q21, q22, [REG_IP], #32
    stp q23, q24, [REG_IP], #32
    stp q25, q26, [REG_IP], #32
    stp q27, q28, [REG_IP], #32
    stp q29, q30, [REG_IP], #32
    stp q31, q16, [REG_IP], #32
    stp q17, q18, [REG_IP], #32
    ret


.align  4
.globl cdecl(__svm_restore_guest_register_x64)
cdecl(__svm_restore_guest_register_x64):
    ldr REG_STATUS, [REG_CTX, OFFSET_X64_STATUS]
    mov REG_IP, REG_CTX
    ldp x19, x20, [REG_IP], #16
    ldp x21, x22, [REG_IP], #16
    ldp x23, x24, [REG_IP], #16
    ldp x25, x26, [REG_IP], #16
    ldp x27, x28, [REG_IP], #16
    ldp x4, x5,   [REG_IP], #16
    ldp x6, x7,   [REG_IP], #16
    ldr x8,       [REG_IP], #8
    add REG_IP, REG_CTX, #OFFSET_X64_VREGS
    ldp q19, q20, [REG_IP], #32
    ldp q21, q22, [REG_IP], #32
    ldp q23, q24, [REG_IP], #32
    ldp q25, q26, [REG_IP], #32
    ldp q27, q28, [REG_IP], #32
    ldp q29, q30, [REG_IP], #32
    ldp q31, q16, [REG_IP], #32
    ldp q17, q18, [REG_IP], #32
    ret

.align  4
.globl cdecl(__svm_interrupt_trampoline_x64)
cdecl(__svm_interrupt_trampoline_x64):
    stp x29, x30, [sp, #-16]!
    bl  cdecl(__svm_save_guest_register_x64)
    mrs REG_IP, nzcv
    str REG_IP_W, [REG_CTX, OFFSET_NZCV]

    bl cdecl(__svm_interrupt_callback_stub)

    ldr REG_IP_W, [REG_CTX, OFFSET_NZCV]
    msr nzcv, REG_IP
    bl  cdecl(__svm_restore_guest_register_x64)
    ldp x29, x30, [sp], #16
    ldr REG_PT, [REG_CTX, OFFSET_PT]

    ldr REG_IP, [REG_CTX, OFFSET_CODE_CACHE]
    str xzr, [REG_CTX, OFFSET_CODE_CACHE]
    cbz REG_IP, label_end
    ldp x29, x30, [sp], #16
    br REG_IP

label_end:
    ret


.align  4
.globl cdecl(__svm_code_run_entry_x64)
cdecl(__svm_code_run_entry_x64):
    stp x29, x30, [sp, #-16]!
    bl cdecl(__svm_save_host_callee)

    bl cdecl(__svm_restore_guest_register_x64)
    ldr REG_IP_W, [REG_CTX, OFFSET_NZCV]
    msr nzcv, REG_IP
    ldr REG_PT, [REG_CTX, OFFSET_PT]

    ldr REG_IP, [REG_CTX, OFFSET_CODE_CACHE]
    str xzr, [REG_CTX, OFFSET_CODE_CACHE]
    blr REG_IP

    mrs REG_IP, nzcv
    str REG_IP_W, [REG_CTX, OFFSET_NZCV]
    bl cdecl(__svm_save_guest_register_x64)

    bl cdecl(__svm_restore_host_callee)
    ldp x29, x30, [sp], #16
    ret


.align  4
.globl cdecl(__svm_call_host_trampoline)
cdecl(__svm_call_host_trampoline):
    stp x29, x30, [sp, #-16]!
    bl  cdecl(__svm_save_host_caller)
    mrs REG_IP, nzcv
    str REG_IP_W, [REG_CTX, OFFSET_NZCV]

    bl cdecl(__svm_call_host_callback_stub)

    ldr REG_IP_W, [REG_CTX, OFFSET_NZCV]
    msr nzcv, REG_IP
    bl  cdecl(__svm_restore_host_caller)
    ldp x29, x30, [sp], #16
    ret


.align  4
.globl cdecl(__svm_cache_miss_trampoline)
cdecl(__svm_cache_miss_trampoline):
    stp x29, x30, [sp, #-16]!
    bl  cdecl(__svm_save_host_caller)
    mrs REG_IP, nzcv
    str REG_IP_W, [REG_CTX, OFFSET_NZCV]

    bl cdecl(__svm_cache_miss_callback)

    ldr REG_IP_W, [REG_CTX, OFFSET_NZCV]
    msr nzcv, REG_IP
    bl  cdecl(__svm_restore_host_caller)
    ldp x29, x30, [sp], #16

    ldr REG_IP, [REG_CTX, OFFSET_CODE_CACHE]
    str xzr, [REG_CTX, OFFSET_CODE_CACHE]
    cbz REG_IP, label_end_cache
    add sp, sp, 16
    br REG_IP

label_end_cache:
    ret


.align  4
.globl cdecl(__svm_bind_cache_trampoline)
cdecl(__svm_bind_cache_trampoline):
    stp x29, x30, [sp, #-16]!
    bl  cdecl(__svm_save_host_caller)
    mrs REG_IP, nzcv
    str REG_IP_W, [REG_CTX, OFFSET_NZCV]

    bl cdecl(__svm_bind_cache_callback)

    ldr REG_IP_W, [REG_CTX, OFFSET_NZCV]
    msr nzcv, REG_IP
    bl  cdecl(__svm_restore_host_caller)
    ldp x29, x30, [sp], #16

    ldr REG_IP, [REG_CTX, OFFSET_CODE_CACHE]
    str xzr, [REG_CTX, OFFSET_CODE_CACHE]
    cbz REG_IP, label_end_bind
    add sp, sp, 16
    br REG_IP

label_end_bind:
    ret


.align  4
.globl cdecl(__svm_return_to_host)
cdecl(__svm_return_to_host):
    ret

