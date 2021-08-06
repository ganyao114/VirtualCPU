//
// Created by swift on 1/10/21.
//

#include <cache/dispatcher_table.h>
#include "jit_context.h"

#define __ masm.

namespace Svm::A64 {

    A64JitContext::A64JitContext(IR::IRBlock *block, JitRuntime *runtime) : ir_block(block), runtime(runtime) {
        configs = runtime->GetConfigs();
        start_pc = block->StartPC();
        pc = start_pc;
        end = block->StartPC() + block->BlockCodeSize();
        masm.EnsureEmitFor(0x10000);
        if (runtime->Guest64Bit()) {
            page_const = &runtime->GetMemory64();
        } else {
            page_const = &runtime->GetMemory32();
        }
        reg_mng.Initialize(block, runtime->GuestArch());
    }

    void A64JitContext::Get(const CPURegister &dst, const CPURegister &src, u8 size_in_byte, bool high) {
        if (!src.IsFPRegister()) {
            switch (size_in_byte) {
                case 1:
                    if (high) {
                        __ Ubfx(dst.X(), src.X(), 8, 8);
                    } else {
                        __ Ubfx(dst.X(), src.X(), 0, 8);
                    }
                    break;
                case 2:
                    if (high) {
                        __ Ubfx(dst.X(), src.X(), 16, 32);
                    } else {
                        __ Ubfx(dst.X(), src.X(), 0, 16);
                    }
                    break;
                case 4:
                    __ Mov(dst.W(), src.W());
                    break;
                case 8:
                    __ Mov(dst.X(), src.X());
                    break;
                default:
                    UNREACHABLE();
            }
        } else {
            UNREACHABLE();
        }
    }

    void A64JitContext::Set(const CPURegister &dst, const CPURegister &src, u8 size_in_byte, bool high) {
        if (!src.IsFPRegister()) {
            switch (size_in_byte) {
                case 1:
                    if (high) {
                        __ Bfi(dst.W(), src.W(), 8, 8);
                    } else {
                        __ Bfi(dst.W(), src.W(), 0, 8);
                    }
                    break;
                case 2:
                    if (high) {
                        __ Bfi(dst.W(), src.W(), 16, 32);
                    } else {
                        __ Bfi(dst.W(), src.W(), 0, 16);
                    }
                    break;
                case 4:
                    __ Mov(dst.W(), src.W());
                    break;
                case 8:
                    __ Mov(dst.X(), src.X());
                    break;
                default:
                    UNREACHABLE();
            }
        } else {
            UNREACHABLE();
        }
    }

    void A64JitContext::Load(const CPURegister &rt, const MemOperand &operand, u8 size_in_byte,
                             bool sign) {
        if (rt.IsVRegister()) {
            switch (size_in_byte) {
                case 1:
                    __ Ldr(rt.B(), operand);
                    break;
                case 2:
                    __ Ldr(rt.H(), operand);
                    break;
                case 4:
                    __ Ldr(rt.S(), operand);
                    break;
                case 8:
                    __ Ldr(rt.D(), operand);
                    break;
                case 16:
                    __ Ldr(rt.Q(), operand);
                    break;
                default:
                    UNREACHABLE();
            }
        } else {
            switch (size_in_byte) {
                case 1:
                    if (sign) {
                        __ Ldrsb((const Register &) rt, operand);
                    } else {
                        if (configs->accurate_memory) {
                            __ Ldrab(rt.W(), operand);
                        } else {
                            __ Ldrb(rt.W(), operand);
                        }
                    }
                    break;
                case 2:
                    if (sign) {
                        __ Ldrsh((const Register &) rt, operand);
                    } else {
                        if (configs->accurate_memory) {
                            __ Ldarh(rt.W(), operand);
                        } else {
                            __ Ldrh(rt.W(), operand);
                        }
                    }
                    break;
                case 4:
                    if (sign) {
                        __ Ldrsw((const Register &) rt, operand);
                    } else {
                        if (configs->accurate_memory) {
                            __ Ldar(rt.W(), operand);
                        } else {
                            __ Ldr(rt.W(), operand);
                        }
                    }
                    break;
                case 8:
                    if (configs->accurate_memory) {
                        __ Ldar(rt.X(), operand);
                    } else {
                        __ Ldr(rt.X(), operand);
                    }
                    break;
                default:
                    abort();
            }
        }
    }

    void A64JitContext::Store(const CPURegister &rt, const MemOperand &operand, u8 size_in_byte) {
        if (rt.IsVRegister()) {
            switch (size_in_byte) {
                case 1:
                    __ Str(rt.B(), operand);
                    break;
                case 2:
                    __ Str(rt.H(), operand);
                    break;
                case 4:
                    __ Str(rt.S(), operand);
                    break;
                case 8:
                    __ Str(rt.D(), operand);
                    break;
                case 16:
                    __ Str(rt.Q(), operand);
                    break;
                default:
                    abort();
            }
        } else {
            switch (size_in_byte) {
                case 1:
                    if (configs->accurate_memory) {
                        __ Stlrb(rt.W(), operand);
                    } else {
                        __ Strb(rt.W(), operand);
                    }
                    break;
                case 2:
                    if (configs->accurate_memory) {
                        __ Stlrh(rt.W(), operand);
                    } else {
                        __ Strh(rt.W(), operand);
                    }
                    break;
                case 4:
                    if (configs->accurate_memory) {
                        __ Stlr(rt.W(), operand);
                    } else {
                        __ Str(rt.W(), operand);
                    }
                    break;
                case 8:
                    if (configs->accurate_memory) {
                        __ Stlr(rt.X(), operand);
                    } else {
                        __ Str(rt.X(), operand);
                    }
                    break;
                default:
                    abort();
            }
        }
    }

    void A64JitContext::CompareAndSwap(const Register &pa, const Register &ex_value,
                                       const Register &new_value, u8 size_in_byte) {
        switch (size_in_byte) {
            case 1:
                __ Casalb(ex_value.W(), new_value.W(), MemOperand(pa));
                break;
            case 2:
                __ Casalh(ex_value.W(), new_value.W(), MemOperand(pa));
                break;
            case 4:
                __ Casal(ex_value.W(), new_value.W(), MemOperand(pa));
                break;
            case 8:
                __ Casal(ex_value, new_value, MemOperand(pa));
                break;
            default:
                abort();
        }
    }

    // cas compat for old device < arm v8.2
    void A64JitContext::CasCompat(const Register &pa, const Register &ex_value, const Register &new_value, u8 size_in_byte) {

        auto ldaxr = [&](const Register &rt, const MemOperand &op) -> void {
            switch (size_in_byte) {
                case 1:
                    __ ldaxrb(rt.W(), op);
                    break;
                case 2:
                    __ ldaxrh(rt.W(), op);
                    break;
                case 4:
                    __ ldaxr(rt.W(), op);
                    break;
                case 8:
                    __ ldaxr(rt, op);
                    break;
                default:
                    abort();
            }
        };

        auto stlxr = [&](const Register &rt, const Register &rs, const MemOperand &op) -> void {
            switch (size_in_byte) {
                case 1:
                    __ stlxrb(rs, rt.W(), op);
                    break;
                case 2:
                    __ stlxrh(rs, rt.W(), op);
                    break;
                case 4:
                    __ stlxr(rs, rt.W(), op);
                    break;
                case 8:
                    __ stlxr(rs, rt, op);
                    break;
                default:
                    abort();
            }
        };

        auto loop = label_alloc.AllocLabel();
        auto done = label_alloc.AllocLabel();
        auto &old_value = reg_mng.AcquireTmpX();
        auto &rs = reg_mng.AcquireTmpX();
        auto &flag = reg_mng.AcquireTmpX();
        __ Msr(NZCV, flag.W());

        __ Bind(loop);
        __ Mov(rs, xzr);
        ldaxr(old_value, MemOperand(pa));
        __ Cmp(old_value, ex_value);
        __ B(Condition::ne, done);
        stlxr(new_value, rs, MemOperand(pa));
        __ Cbnz(rs, loop);

        __ Bind(done);
        __ Mrs(flag.W(), NZCV);
        reg_mng.ReleaseTmpX(rs);
        reg_mng.ReleaseTmpX(flag);
        reg_mng.ReleaseTmpX(old_value);
    }

    void A64JitContext::ReadMemory(const CPURegister &rt, VirtualAddress va, bool atomic) {
        auto mmu_on = page_const;
        if (!mmu_on) {
            Register address{};
            if (va.Const()) {
                address = reg_mng.IP();
                __ Mov(address, va.Address());
            } else {
                address = va.Reg();
            }
            Load(rt, MemOperand(address), va.SizeInByte());
            return;
        }
        // fake mmu
        if (configs->use_offset_pt) {
            if (va.Const()) {
                auto &tmp = reg_mng.IP();
                __ Mov(tmp, va.Address());
                Load(rt, MemOperand(reg_mng.PageTale(), tmp), va.SizeInByte());
            } else {
                Load(rt, MemOperand(reg_mng.PageTale(), va.Address()), va.SizeInByte());
            }
            return;
        }
        CheckPageOverflow(va, rt, Memory::PageEntry::Read, atomic);
        auto &pa = reg_mng.AcquireTmpX();
        VALookup(pa, va, Memory::PageEntry::Read);
        Load(rt, MemOperand(pa), va.SizeInByte());
        reg_mng.ReleaseTmpX(pa);
    }

    void A64JitContext::WriteMemory(const CPURegister &rt, VirtualAddress va, bool atomic) {
        auto mmu_on = page_const;
        if (!mmu_on) {
            Register address{};
            if (va.Const()) {
                address = reg_mng.IP();
                __ Mov(address, va.Address());
            } else {
                address = va.Reg();
            }
            Store(rt, MemOperand(address), va.SizeInByte());
            return;
        }
        // fake mmu
        if (configs->use_offset_pt) {
            if (va.Const()) {
                auto &tmp = reg_mng.IP();
                __ Mov(tmp, va.Address());
                Store(rt, MemOperand(reg_mng.PageTale(), tmp), va.SizeInByte());
            } else {
                Store(rt, MemOperand(reg_mng.PageTale(), va.Address()), va.SizeInByte());
            }
            return;
        }
        CheckPageOverflow(va, rt, Memory::PageEntry::Write, atomic);
        auto &pa = reg_mng.AcquireTmpX();
        VALookup(pa, va, Memory::PageEntry::Write);
        Store(rt, MemOperand(pa), va.SizeInByte());
        reg_mng.ReleaseTmpX(pa);
    }

    void A64JitContext::MemBarrier() {
        __ Dmb(FullSystem, BarrierAll);
    }

    void A64JitContext::VALookup(const Register &pa, VirtualAddress va, u8 perms) {

        auto &ctx = reg_mng.Context();
        auto &pt = reg_mng.PageTale();
        auto &ip = reg_mng.IP();

        // fake mmu
        if (configs->use_offset_pt) {
            if (va.Const()) {
                __ Mov(pa, va.Address() + reinterpret_cast<VAddr>(configs->offset_pt_base));
            } else {
                __ Add(pa, pt, va.Reg());
            }
            return;
        }

        if (va.Const()) {
            __ Mov(pa, va.Address() >> page_const->page_bits);
            __ Ldr(pa, MemOperand(pt, pa, LSL, 3));
        } else {
            __ Lsr(pa, va.Reg(), page_const->page_bits);
            __ Ldr(pa, MemOperand(pt, pa, LSL, 3));
        }

        auto page_fail_label = BindPageFatal(va, perms).fatal_label;

        if (perms & Memory::PageEntry::Read) {
            __ Tbz(pa, Memory::readable_bit, page_fail_label);
        }

        if (perms & Memory::PageEntry::Write) {
            __ Tbz(pa, Memory::writable_bit, page_fail_label);
        }

        if (perms & Memory::PageEntry::Exe) {
            __ Tbz(pa, Memory::executable_bit, page_fail_label);
        }

        if (va.Const()) {
            __ Bfc(pa, 0, page_const->page_bits);
            __ Add(pa, pa, va.Address() & page_const->page_mask);
        } else {
            __ Bfi(pa, va.Reg(), 0, page_const->page_bits);
        }
    }

    void A64JitContext::CheckPageOverflow(VirtualAddress &va, const CPURegister &rt, u8 action, bool atomic) {
        auto &ctx = reg_mng.Context();
        auto &pt = reg_mng.PageTale();
        auto &ip = reg_mng.IP();

        // check page align
        if (!configs->use_offset_pt && configs->page_align_check && !atomic && va.CheckAlign()) {
            if (va.Const()) {
                // page overflow
                if ((va.Address() & page_const->page_mask) + va.SizeInByte() > page_const->page_size) {
                    auto &page_fallback = AllocPageFallback(va, rt, action, atomic);
                    __ B(page_fallback.fall_label);
                }
            } else {
                auto &page_fallback = AllocPageFallback(va, rt, action, atomic);
                __ Ubfx(ip.W(), va.Reg(), 0, page_const->page_bits);
                __ Add(ip.W(), ip.W(), va.SizeInByte() - 1);
                __ Tbnz(ip, page_const->page_bits + 1, page_fallback.fall_label);
            }
        }
    }

    void A64JitContext::CodeCacheLookup(const Register &va, const Register &pa, Label *miss_cache) {
        auto label_loop = label_alloc.AllocLabel();
        auto &tmp1 = reg_mng.AcquireTmpX();
        auto &tmp2 = pa;
        auto &ctx = reg_mng.Context();
        // load hash table
        __ Ldr(tmp1, MemOperand(ctx, OFF_HELP(dispatcher_table)));
        // align
        __ Lsr(tmp2, va, 2);
        // hash
        __ Eor(tmp2, tmp2, Operand(tmp2, LSR, HASH_TABLE_PAGE_BITS));
        __ And(tmp2, tmp2, (1 << HASH_TABLE_PAGE_BITS) - 1);
        // looper
        __ Add(tmp1, tmp1, Operand(tmp2, LSL, 4));
        __ Bind(label_loop);
        __ Ldr(tmp2, MemOperand(tmp1, 16, PostIndex));
        __ Cbz(tmp2, miss_cache);
        __ Sub(tmp2, tmp2, va);
        __ Cbnz(tmp2, label_loop);
        // find target
        __ Ldr(pa, MemOperand(tmp1, -8, PreIndex));
        reg_mng.ReleaseTmpX(tmp1);
    }

    void A64JitContext::CheckHalt() {
        if (!configs->check_halt) {
            return;
        }
        auto &tmp = reg_mng.IP();
        auto &ctx = reg_mng.Context();
        __ Ldrb(tmp, MemOperand(ctx, OFF_HELP(halt_flag)));
        __ Cbnz(tmp, &return_to_host);
    }

    void A64JitContext::Forward(VAddr va) {
        auto &ip = reg_mng.IP();
        auto &ctx = reg_mng.Context();
        __ Mov(ip, va);
        Store<VAddr>(ip, MemOperand(ctx, reg_mng.PCOffset()));
        CheckHalt();
        auto [base, block] = runtime->GetBlockCache(va);
        VAddr cache;
        if (block->BoundModule()) {
            auto &module_info = block->GetModule();
            auto module = runtime->GetModule(module_info.module_id);
            auto buffer = module->GetBoundBuffer(module_info.block_id);
            cache = reinterpret_cast<VAddr>(buffer.stub_data);
            __ B(label_alloc.AllocOutstanding(cache));
        } else {
            auto dispatcher_index = block->GetDispatchIndex();
            __ Ldr(ip, MemOperand(ctx, OFF_HELP(dispatcher_table)));
            auto tmp = reg_mng.AcquireTmpX();
            __ Mov(tmp, dispatcher_index * 2 + 1);
            __ Ldr(ip, MemOperand(ip, tmp, LSL, 3));
            __ Br(ip);
            reg_mng.ReleaseTmpX(tmp);
        }
//        if (cache) {
//            __ B(label_alloc.AllocOutstanding(cache));
//        } else {
//            auto miss = label_alloc.AllocLabel();
//            auto &va_reg = reg_mng.AcquireTmpX();
//            __ Mov(va_reg, ip);
//            CodeCacheLookup(va_reg, ip, miss);
//            reg_mng.ReleaseTmpX(va_reg);
//            __ Br(ip);
//            __ Bind(miss);
//            // go to host
//            __ Ldr(ip, MemOperand(ctx, OFF_HELP(cache_miss_trampoline)));
//            __ Br(ip);
//        }
    }

    void A64JitContext::Forward(const Register &va, bool check_rsb) {
        auto &ctx = reg_mng.Context();
        // store pc
        Store<VAddr>(va, MemOperand(ctx, reg_mng.PCOffset()));
        CheckHalt();
        auto miss_cache = label_alloc.AllocLabel();
        auto &pa = reg_mng.IP();
        if (check_rsb && configs->rsb_cache) {
            auto miss_rsb = label_alloc.AllocLabel();
            PopRSB(va, pa, miss_cache);
            __ Br(pa);
            __ Bind(miss_rsb);
        }
        CodeCacheLookup(va, pa, miss_cache);
        __ Br(pa);
        __ Bind(miss_cache);
        // go to host
        ReturnHost();
    }

    void A64JitContext::ReturnHost() {
        __ Ret();
    }

    void A64JitContext::PushRSB(VAddr va) {
        if (!configs->rsb_cache) {
            return;
        }
        auto [base, block] = runtime->GetBlockCache(va);
        VAddr cache{};
        if (block->Compiled()) {
            cache = runtime->GetBlockCodeCache(block);
        }
        auto &ip = reg_mng.IP();
        auto &ctx = reg_mng.Context();
        Load<u8>(ip, MemOperand(ctx, OFF_HELP(rsb_cursor)));
        // inc cursor
        __ Add(ip, ctx, sizeof(VAddr));
        Store<u8>(ip, MemOperand(ctx, OFF_HELP(rsb_cursor)));
        __ Add(ip, ctx, Operand(ip, LSL, 2));
        __ Add(ip, ip, OFF_HELP(rsb_cache));
        if (cache) {
            auto &tmp = reg_mng.AcquireTmpX();
            __ Mov(tmp, va);
            Store<u8>(tmp, MemOperand(ip, sizeof(VAddr), PostIndex));
            __ Mov(tmp, cache);
            Store<u8>(ip, MemOperand(ip));
            reg_mng.ReleaseTmpX(tmp);
        } else {
            Store<VAddr>(xzr, MemOperand(ctx, OFF_HELP(rsb_cursor)));
        }
    }

    void A64JitContext::PopRSB(const Register &va, const Register &pa, Label *miss_cache) {
        auto &ctx = reg_mng.Context();
        auto &tmp = reg_mng.AcquireTmpX();
        Load<u8>(tmp, MemOperand(ctx, OFF_HELP(rsb_cursor)));
        __ Add(pa, ctx, OFF_HELP(rsb_cache));
        Load<VAddr>(pa, MemOperand(pa, tmp, LSL, 2));
        // dec cursor
        __ Sub(tmp, tmp, sizeof(VAddr));
        Store<u8>(tmp, MemOperand(ctx, OFF_HELP(rsb_cursor)));
        __ Sub(pa, va, pa);
        __ Cbnz(pa, miss_cache);
        __ Add(pa, ctx, OFF_HELP(rsb_cache) + sizeof(RSBEntry) + sizeof(VAddr));
        Load<VAddr>(pa, MemOperand(pa, tmp, LSL, 2));
        reg_mng.ReleaseTmpX(tmp);
    }

    void A64JitContext::RaiseException(Exception::Action action, VAddr pc_, const Register& data) {
        auto &ip = reg_mng.IP();
        auto &ctx = reg_mng.Context();
        if (data.IsValid()) {
            Store<u64>(data, MemOperand(ctx, OFF_HELP(exception.action)));
        }
        __ Mov(ip, pc_);
        Store<VAddr>(ip, MemOperand(ctx, reg_mng.PCOffset()));
        __ Mov(ip, action.raw);
        Store<Exception::Action>(ip, MemOperand(ctx, OFF_HELP(exception.action)));
        __ Stp(x29, x30, MemOperand(sp, -16, PreIndex));
        __ Ldr(reg_mng.IP(), MemOperand(reg_mng.Context(), OFF_HELP(interrupt_trampoline)));
        __ Blr(reg_mng.IP());
        __ Ldp(x29, x30, MemOperand(sp, 16, PostIndex));
    }

    void A64JitContext::CallFunction(u32 func_offset, const Register& ret, const Vector<const Register> &args) {
        CallHost::FuncType func{};
        func.func_offset = func_offset;
        func.ret_void = ret.IsValid();
        func.argc = args.size();
        auto &tmp = reg_mng.AcquireTmpX();
        __ Mov(tmp.W(), func.raw);
        __ Str(tmp.W(), MemOperand(reg_mng.Context(), OFF_HELP(host_call.func)));
        reg_mng.ReleaseTmpX(tmp);
        // push args
        auto offset_argv = OFF_HELP(host_call.argv);
        for (auto &arg : args) {
            __ Str(arg, MemOperand(reg_mng.Context(), offset_argv));
            offset_argv += sizeof(void*);
        }
        // to trampoline
        __ Stp(x29, x30, MemOperand(sp, -16, PreIndex));
        __ Ldr(reg_mng.IP(), MemOperand(reg_mng.Context(), OFF_HELP(call_host_trampoline)));
        __ Blr(reg_mng.IP());
        __ Ldp(x29, x30, MemOperand(sp, 16, PostIndex));
        if (!ret.IsValid()) {
            __ Ldr(ret, MemOperand(reg_mng.Context(), OFF_HELP(host_call.ret_val)));
        }
    }

    void A64JitContext::FlushFallback() {
        auto label_continue = label_alloc.AllocLabel();
        __ B(label_continue);

        auto &ctx = reg_mng.Context();
        auto &ip = reg_mng.IP();
        // build page fallback
        for (auto &pf : page_fallbacks) {
            __ Bind(pf.fall_label);
            __ Brk(0);
        }

        // build page fatal
        auto label_interrupt = label_alloc.AllocLabel();
        for (auto &pf : page_fatals) {
            __ Bind(pf.fatal_label);
            Exception::Action action{};
            action.reason = Exception::PAGE_FATAL;
            action.flag = pf.perms;
            __ Mov(ip, action.raw);
            Store<Exception::Action>(ip, MemOperand(ctx, OFF_HELP(exception.action)));
            if (pf.va.Const()) {
                __ Mov(ip, pf.va.Address());
                Store<VAddr>(ip, MemOperand(ctx, OFF_HELP(exception.data)));
            } else {
                Store<VAddr>(pf.va.Reg(), MemOperand(ctx, OFF_HELP(exception.data)));
            }
            __ Adr(ip, pf.rewind_label);
            Store<VAddr>(ip, MemOperand(ctx, OFF_HELP(exception.rewind)));
            __ Mov(ip, pc);
            __ B(label_interrupt);
        }
        __ Bind(label_interrupt);
        Store<VAddr>(ip, MemOperand(ctx, reg_mng.PCOffset()));
        __ Stp(x29, x30, MemOperand(sp, -16, PreIndex));
        __ Ldr(reg_mng.IP(), MemOperand(reg_mng.Context(), OFF_HELP(interrupt_trampoline)));
        __ Blr(reg_mng.IP());
        __ Ldp(x29, x30, MemOperand(sp, 16, PostIndex));
        Load<VAddr>(ip, MemOperand(ctx, OFF_HELP(exception.rewind)));
        __ Br(ip);

        __ Bind(label_continue);

        page_fatals.clear();
        page_fallbacks.clear();
    }

    void A64JitContext::EndBlock() {
        FlushFallback();
        // build return host
        __ Bind(&return_to_host);
        ReturnHost();
    }

    void A64JitContext::Flush(CodeBuffer &buffer) {
        label_alloc.SetDestBuffer(reinterpret_cast<VAddr>(buffer.exec_data));
        __ FinalizeCode();
    }

    PageFallback &A64JitContext::AllocPageFallback(VirtualAddress &va, const CPURegister &reg, u8 action, bool atomic) {
        auto &res = page_fallbacks.emplace_back(va);
        res.fall_label = label_alloc.AllocLabel();
        res.rewind_label = label_alloc.AllocLabel();
        res.action = action;
        res.atomic = atomic;
        res.pc = pc;
        res.rt = reg;
        return res;
    }

    PageFatal &A64JitContext::BindPageFatal(VirtualAddress &va, u8 perms) {
        auto &res = page_fatals.emplace_back(va);
        res.fatal_label = label_alloc.AllocLabel();
        res.rewind_label = label_alloc.AllocLabel();
        res.perms = perms;
        res.pc = pc;
        __ Bind(res.rewind_label);
        return res;
    }

    void A64JitContext::TickIR(u32 ir_id) {
        reg_mng.TickIR(ir_id);
    }

    void A64JitContext::TickPC(VAddr pc_) {
        this->pc = pc_;
    }
}

#undef __