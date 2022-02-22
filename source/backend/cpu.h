//
// Created by swift on 1/10/21.
//

#pragma once

#include <include/types.h>
#include <frontend/x64/cpu.h>
#include "backend/arm64/constants.h"

#define OFF_HELP(name) OFFSET_OF(Svm::CPUContext, help.name)

namespace Svm {

    struct CpuFlags {
        BitField<0, 1, u32> n;
        BitField<1, 1, u32> z;
        BitField<2, 1, u32> c;
        BitField<3, 1, u32> v;
        BitField<8, 1, u32> p;
        BitField<9, 1, u32> a;
        BitField<10, 1, u32> d;
    };

    struct RSBEntry {
        VAddr va;
        void *cache;
    };

    struct Exception {

        enum Reason : u8 {
            NONE = 0,
            FALLBACK,
            SVC,
            BRK,
            YIELD,
            HLT,
            ILL_CODE,
            PAGE_FATAL,
            HALT
        };

        union Action {
            u16 raw;
            struct {
                Reason reason;
                u8 flag;
            };

            explicit Action(Reason reason, u8 flag) : reason(reason), flag(flag) {};

            explicit Action(Reason reason) : reason(reason), flag(0) {};
        };

        Action action{NONE};
        u64 data;
        VAddr rewind;

        template<typename T>
        constexpr T Data() {
            return static_cast<T>(data);
        }
    };

    struct CallHost {

        union FuncType {
            u32 raw;
            struct {
                u32 func_offset: 26;
                u32 argc: 5;
                u32 ret_void: 1;
            };
        };

        VAddr callback_base;
        FuncType func;

        union {
            Array<void *, 4> argv;
            void *ret_val;
        };
    };

    constexpr auto rsb_stack_size = 32;

    struct HelpContext {
        void *context_ptr;
        void *code_cache;
        void *dispatcher_table;
        // memory
        void *page_table;
        // host stubs
        void *host_stubs;
        void *interrupt_sp;
        void *host_sp;
        // ticks
        u64 ticks_now;
        u64 ticks_max;
        // help fields
        CpuFlags cpu_flags{};
        // exception
        Exception exception;
        // call host
        CallHost host_call;
        // trampolines
        void *call_host_trampoline;
        void *interrupt_trampoline;
        void *cache_miss_trampoline;
        // halt flag
        bool halt_flag{false};
        // return stack buffer block
        u8 rsb_cursor;
        Array<RSBEntry, rsb_stack_size> rsb_cache{};
    };

    struct CPUContext {
        Array<u8, 1024> guest_ctx_data{};
        HelpContext help{};

        constexpr HelpContext &Helper() {
            return help;
        }

        constexpr X86::ThreadContext64 *X64() {
            return (X86::ThreadContext64 *)guest_ctx_data.data();
        }
    };

}
