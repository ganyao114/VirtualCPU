//
// Created by swift on 2021/6/12.
//

#pragma once

#include "block.h"
#include <include/memory_interface.h>
#include <base/array_ref.h>
#include <frontend/x64/cpu.h>

namespace Svm {
    class JitRuntime;
    class VCpu;
}

namespace Svm::IR {

    struct IRReg {

        bool is_float;
        bool carry{false};
        bool overflow{false};
        u128 data;

        template<typename T>
        inline T Get() {
            return static_cast<T>(data[0]);
        }

        template<typename T>
        inline T Get() const {
            return static_cast<T>(data[0]);
        }

        template<typename T>
        inline T GetF() const {
            return static_cast<T>(data);
        }

        template<typename T>
        constexpr void Set(T val) {
            auto data_set = reinterpret_cast<T *>(data.data());
            *data_set = val;
        }

    };

    class Interpreter {
    public:

        explicit Interpreter(JitRuntime *runtime, IRBlock *block, VCpu *core, void *inter_stack);

        virtual ~Interpreter();

        void Run();

    private:

        void Terminal();

        constexpr bool NeedTestOverflow(IR::Instruction *instr) {
            return instr->HasCalAct() && instr->GetCalAct().overflow;
        }

        template <typename T>
        bool OverflowCheck(u64 left, u64 right, u64 result) {
            constexpr auto sign_mask = T(1) << (sizeof(T) * 8 - 1);
            u64 left_sign = left & sign_mask;
            u64 right_sign = right & sign_mask;
            u64 result_sign = result & sign_mask;
            return (left_sign == right_sign) && (left_sign != result_sign);
        }

        template <typename T>
        void TestOverflow(Instruction *instr, u64 left, u64 right, u64 result) {
            if (NeedTestOverflow(instr)) {
                Ret(instr).overflow = OverflowCheck<T>(left, right, result);
            }
        }

        void TestOverflow(Instruction *instr, u64 left, u64 right, u64 result) {
            if (NeedTestOverflow(instr)) {
                auto &ret = instr->GetReturn().Get<Value>();
                switch (ret.GetSize()) {
                    case U8:
                        TestOverflow<u8>(instr, left, right, result);
                        break;
                    case U16:
                        TestOverflow<u16>(instr, left, right, result);
                        break;
                    case U32:
                        TestOverflow<u32>(instr, left, right, result);
                        break;
                    case U64:
                        TestOverflow<u64>(instr, left, right, result);
                        break;
                    default:
                        UNREACHABLE();
                }
            }
        }

        template <typename Expr, bool float_ = false>
        constexpr void Operate(Expr expr, IR::Value &left, IR::Imm &right) {
            auto &ret_val = current->GetReturn().Get<Value>();
            switch (left.GetSize()) {
                case IR::Size::U8:
                    V(ret_val).Set(expr(V(left).Get<u8>(), right.Value<u8>()));
                    break;
                case IR::Size::U16:
                    V(ret_val).Set(expr(V(left).Get<u16>(), right.Value<u16>()));
                    break;
                case IR::Size::U32:
                    V(ret_val).Set(expr(V(left).Get<u32>(), right.Value<u32>()));
                    break;
                case IR::Size::U64:
                    V(ret_val).Set(expr(V(left).Get<u64>(), right.Value<u64>()));
                    break;
                case IR::Size::U128:
                    if constexpr (float_) {
                        V(ret_val).Set(expr(V(left).GetF<u128>(), right.Value<u64>()));
                    }
                    break;
                default:
                    UNREACHABLE();
            }
        }

        template <typename Expr, bool float_ = false>
        constexpr void Operate(Expr expr, IR::Value &left, IR::Value &right) {
            auto &ret_val = current->GetReturn().Get<Value>();
            switch (left.GetSize()) {
                case IR::Size::U8:
                case IR::Size::U16:
                case IR::Size::U32:
                case IR::Size::U64:
                    Set(ret_val, expr(Get(left), Get(right)));
                    break;
                case IR::Size::U128:
                    if constexpr (float_) {
                        V(ret_val).Set(expr(V(left).GetF<u128>(), V(right).Get<u128>()));
                    }
                    break;
                default:
                    UNREACHABLE();
            }
        }

        constexpr void Set(IR::Value &value, u64 data) {
            switch (value.GetSize()) {
                case IR::Size::U8:
                    V(value).Set<u8>(data);
                    break;
                case IR::Size::U16:
                    V(value).Set<u16>(data);
                    break;
                case IR::Size::U32:
                    V(value).Set<u32>(data);
                    break;
                case IR::Size::U64:
                    V(value).Set<u64>(data);
                    break;
                default:
                    UNREACHABLE();
            }
        }

        constexpr u64 Get(IR::Value &value) {
            switch (value.GetSize()) {
                case IR::Size::U8:
                    return V(value).Get<u8>();
                case IR::Size::U16:
                    return V(value).Get<u16>();
                case IR::Size::U32:
                    return V(value).Get<u32>();
                case IR::Size::U64:
                    return V(value).Get<u64>();
                default:
                    UNREACHABLE();
            }
        }

        constexpr IRReg &V(IR::Value &value) {
            return regs[value.Def()->GetId()];
        }

        constexpr IRReg &Ret(Instruction *instr) {
            return V(instr->GetReturn().Get<Value>());
        }

#define INST0(name, ret) void name();
#define INST1(name, ret, arg1) void name(IR::arg1& a1);
#define INST2(name, ret, arg1, arg2) void name(IR::arg1& a1, IR::arg2& a2);
#define INST3(name, ret, arg1, arg2, arg3) void name(IR::arg1& a1, IR::arg2& a2, IR::arg3& a3);
#define INST4(name, ret, arg1, arg2, arg3, arg4) void name(IR::arg1& a1, IR::arg2& a2, IR::arg3& a3, IR::arg4& a4);

#include <ir/instr_table.ir>

#undef INST0
#undef INST1
#undef INST2
#undef INST3
#undef INST4

        bool CheckCondX64(IR::Cond &cond);

        JitRuntime *runtime;
        VCpu *core;
        union {
            X86::ThreadContext64 *x64_ctx;
        };
        IR::IRBlock *ir_block;
        IR::Instruction *current{};
        Memory::MemoryInterface *memory;
        u32 skip_to_id{};
        u32 block_offset{};
        ArrayRef<IRReg> regs{};
        void *interp_stack_mem{};
    };

}
