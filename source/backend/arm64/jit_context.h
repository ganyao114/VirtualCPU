//
// Created by swift on 1/10/21.
//

#pragma once

#include <base/marco.h>
#include "backend/cpu.h"
#include <externals/vixl/aarch64/assembler-aarch64.h>
#include <externals/vixl/aarch64/macro-assembler-aarch64.h>
#include <ir/block.h>
#include <runtime/page_table.h>
#include <include/configs.h>
#include <runtime/jit_runtime.h>
#include <backend/arm64/host_callbacks.h>
#include "register_manager.h"
#include "label_manager.h"
#include "host_callbacks.h"

#define FUNC(name) OFFSET_OF(HostCallbacks, name)

namespace Svm::A64 {

    using namespace vixl::aarch64;
    class A64JitContext;

    class VirtualAddress final {
    public:

        constexpr VirtualAddress(const Register &rt) : rt{rt}, vaddr(0), const_addr{false}, size{0} {};

        constexpr VirtualAddress(const VAddr &vaddr) : rt{x0}, vaddr(vaddr), const_addr{true}, size{0} {};

        constexpr VirtualAddress(const Register &rt, u8 size_in_byte) : rt{rt}, vaddr(0), const_addr{false}, size{size_in_byte} {};

        constexpr VirtualAddress(const VAddr vaddr, u8 size_in_byte) : rt{x0}, vaddr(vaddr), const_addr{true}, size{size_in_byte} {};

        [[nodiscard]] constexpr bool Const() const {
            return const_addr;
        }

        [[nodiscard]] constexpr VAddr Address() const {
            return vaddr;
        }

        [[nodiscard]] const Register &Reg() const {
            return rt;
        }

        [[nodiscard]] constexpr u8 SizeInByte() const {
            return size;
        }

        [[nodiscard]] constexpr bool CheckAlign() const {
            return SizeInByte() > 1;
        }

    private:
        const Register &rt;
        const VAddr vaddr;
        const bool const_addr;
        const u8 size;
    };

    struct PageFallback {

        constexpr PageFallback(VirtualAddress &va) : va(va) {};

        Label *fall_label{};
        Label *rewind_label{};
        VAddr pc{};
        CPURegister rt{NoReg};
        u8 action{};
        bool atomic{};
        VirtualAddress va;
    };

    struct PageFatal {

        constexpr PageFatal(VirtualAddress &va) : va(va) {};

        Label *fatal_label{};
        Label *rewind_label{};
        u8 perms{};
        VirtualAddress va;
        VAddr pc{};
    };

    struct LinkedLabel {
        Label *label;
        VAddr target;
    };

    class ScopeMemOperand {
    public:

        explicit ScopeMemOperand(A64JitContext *context);

        void Set(const MemOperand &operand);

        MemOperand Get();

        const Register &AcquireTmpX();

        virtual ~ScopeMemOperand();

        ScopeMemOperand(const ScopeMemOperand &old);

        ScopeMemOperand &operator=(const ScopeMemOperand &old);

    private:
        A64JitContext *context;
        MemOperand operand;
        mutable List<Register> tmps;
    };

    class A64JitContext : public CopyDisable {
    public:

        explicit A64JitContext(IR::IRBlock *ir_block, Runtime *runtime, MacroAssembler &masm);

        constexpr bool EnabledMMU() {
            return page_const;
        }

        void Get(const CPURegister &dst, const CPURegister &src, u8 size_in_byte, bool high = false);

        void Set(const CPURegister &dst, const CPURegister &src, u8 size_in_byte, bool high = false);

        void Load(const CPURegister &rt, const MemOperand &operand, u8 size_in_byte, bool sign = false);

        void Store(const CPURegister &rt, const MemOperand &operand, u8 size_in_byte);

        void CompareAndSwap(const Register &pa, const Register &ex_value, const Register &new_value, u8 size_in_byte);

        template <typename T, bool sign = false>
        void Load(const CPURegister &rt, const MemOperand &operand) {
            Load(rt, operand, sizeof(T), sign);
        }

        template <typename T>
        void Store(const CPURegister &rt, const MemOperand &operand) {
            Store(rt, operand, sizeof(T));
        }

        template <typename T>
        void CompareAndSwap(const Register &pa, const Register &ex_value, const Register &new_value) {
            CompareAndSwap(pa, ex_value, new_value, sizeof(T));
        }

        void ReadMemory(const CPURegister &rt, VirtualAddress va, bool atomic = false);

        void WriteMemory(const CPURegister &rt, VirtualAddress va, bool atomic = false);

        void MemBarrier();

        ScopeMemOperand VALookup(VirtualAddress va, u8 perms, const Register &pa = NoReg);

        void CheckPageOverflow(VirtualAddress &va, const CPURegister &rt, u8 action, bool atomic);

        void CodeCacheLookup(const Register &va, const Register &pa, Label *miss_cache);

        void CheckHalt();

        void ReturnHost();

        void Forward(VAddr va);

        void Forward(const Register &va, bool check_rsb = false);

        void CallFunction(u32 func_offset, const Register& ret, const std::vector<const Register> &args);

        void RaiseException(Exception::Action action, VAddr pc, const Register& data = NoReg);

        void PushRSB(VAddr va);

        void PopRSB(const Register &va, const Register &pa, Label *miss_cache);

        void TickIR(u32 ir_id);

        void TickPC(VAddr pc);

        void FlushFallback();

        void EndBlock();

        void Flush(CodeBuffer &buffer);

        constexpr RegisterManager &RegMng() {
            return reg_mng;
        }

        constexpr LabelAllocator &LabelAlloc() {
            return label_alloc;
        }

        constexpr MacroAssembler &Masm() {
            return masm;
        }

        constexpr std::list<LinkedLabel> &LinkLabels() {
            return link_labels;
        }

        constexpr JitRuntime *Runtime() {
            return dynamic_cast<JitRuntime *>(runtime);
        }

        constexpr VAddr PC() const {
            return pc;
        }

        constexpr VAddr BlockEnd() const {
            return end;
        }

        constexpr void MarkCheckHalt() {
            need_check_halt = true;
        }

    private:

        PageFallback &AllocPageFallback(VirtualAddress &va, const CPURegister &reg, u8 action, bool atomic = false);

        PageFatal &BindPageFatal(VirtualAddress &va, u8 perms);

        void CasCompat(const Register &pa, const Register &ex_value, const Register &new_value, u8 size_in_byte);

        Label *DispatcherLabel();

        Label *BlockLabel(VAddr vaddr);

        Label *LinkLabel(VAddr link_target);

        void BuildDispatcher();

        MacroAssembler &masm;
        IR::IRBlock *ir_block{};
        RegisterManager reg_mng{};
        LabelAllocator label_alloc{masm};
        RuntimeLabels *runtime_labels{};
        Memory::PageTableConst *page_const;
        UserConfigs *configs;
        class Runtime *runtime;

        VAddr start_pc{};
        VAddr pc{};
        VAddr end{};

        bool need_check_halt{false};
        bool inline_dispatcher{false};
        bool need_build_dispatcher{false};

        Label return_to_host;
        Label dispatcher_label;
        Label halt_label;
        std::list<PageFallback> page_fallbacks;
        std::list<PageFatal> page_fatals;

        std::list<LinkedLabel> link_labels;
    };

}
