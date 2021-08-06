//
// Created by swift on 1/11/21.
//

#include "register_manager.h"

namespace Svm::A64 {

    void RegisterManager::Initialize(IR::IRBlock *blk, CpuArch guest) {
        this->block = blk;
        this->arch = guest;
        switch (guest) {
            case CpuArch::X64:
                reg_live_interval.resize(x64_guest_tmps.size());
                vreg_live_interval.resize(x64_guest_vtmps.size());
                break;
            default:
                break;
        }
    }

    void RegisterManager::TickIR(u32 id) {
        current_ir_id = id;
    }

    void RegisterManager::DefineValue(IR::Value &value) {
        auto &instr = block->Instr(value.GetId());
        auto reg_be_set = registers_allocated.find(value.GetId());
        if (reg_be_set == registers_allocated.end()) {
            define_to_use_interval[value.GetId()] = {value.GetId(), value.GetId()};
        }
    }

    void RegisterManager::DefineFloatValue(IR::Value &value) {
        auto &instr = block->Instr(value.GetId());
        auto vreg_be_set = vregisters_allocated.find(value.GetId());
        if (vreg_be_set == vregisters_allocated.end()) {
            define_to_use_interval[value.GetId()] = {value.GetId(), value.GetId()};
        }
    }

    void RegisterManager::UseValue(u32 instr_id, IR::Value &value) {
        if (registers_allocated.find(value.GetId()) == registers_allocated.end()) {
            auto &old = define_to_use_interval[value.GetId()];
            old = {old.first, instr_id};
        }
    }

    void RegisterManager::UseFloatValue(u32 instr_id, IR::Value &value) {
        if (vregisters_allocated.find(value.GetId()) == vregisters_allocated.end()) {
            auto &old = define_to_use_interval[value.GetId()];
            old = {old.first, instr_id};
        }
    }

    void RegisterManager::AllocateForBlock() {
        for (auto &itr : define_to_use_interval) {
            auto define_value_id = itr.first;
            auto[value_define_pos, value_end_pos] = itr.second;
            auto &value = block->Instr(define_value_id).GetReturn().Get<IR::Value>();
            if (!value.IsFloat()) {
                auto temp_index = *AllocInRegList(reg_live_interval, value_define_pos, value_end_pos);
                registers_allocated.emplace(define_value_id, x64_guest_tmps[temp_index]);
            } else {
                auto temp_index = *AllocInRegList(reg_live_interval, value_define_pos, value_end_pos);
                vregisters_allocated.emplace(define_value_id, x64_guest_vtmps[temp_index]);
            }
        }
    }

    const Register &RegisterManager::GetValueRegister(IR::Value &value) {
        return registers_allocated[value.GetId()];
    }

    const VRegister &RegisterManager::GetValueVRegister(IR::Value &value) {
        return vregisters_allocated[value.GetId()];
    }

    bool RegisterManager::CastHostReg(IR::Reg &reg) {
        switch (arch) {
            case CpuArch::X64:
                return reg.code <= 14 && reg.type == 0;
            default:
                return false;
        }
    }

    bool RegisterManager::CastHostReg(IR::VReg &reg) {
        switch (arch) {
            case CpuArch::X64:
                return reg.type == 0;
            default:
                return false;
        }
    }

    bool RegisterManager::DirectSetHostReg(IR::Instruction *instr) {
        return true;
    }

    bool RegisterManager::DirectGetHostReg(IR::Instruction *instr, u8 op_index) {
        return true;
    }

    bool RegisterManager::CastedToHostReg(const IR::Reg &reg) {
        switch (arch) {
            case CpuArch::X64:
                return reg.code <= 14;
            default:
                return false;
        }
    }

    bool RegisterManager::CastedToHostVReg(const IR::VReg &reg) {
        switch (arch) {
            case CpuArch::X64:
                return true;
            default:
                return false;
        }
    }

    const VRegister &RegisterManager::ToHostVRegister(IR::VReg &reg) {
        switch (arch) {
            case CpuArch::X64: {
                auto &a64 = x64_to_a64_vregs_table[reg.GetCode()].a64;
                if (reg.size == IR::Size::U128) {
                    return a64.Q();
                } else if (reg.size == IR::Size::U64) {
                    return a64.D();
                } else if (reg.size == IR::Size::U32) {
                    return a64.S();
                } else if (reg.size == IR::Size::U16) {
                    return a64.H();
                } else if (reg.size == IR::Size::U8) {
                    return a64.B();
                } else {
                    UNREACHABLE();
                }
                break;
            }
            default:
                UNREACHABLE();
        }
    }

    const Register &RegisterManager::ToHostRegister(IR::Reg &reg) {
        switch (arch) {
            case CpuArch::X64: {
                auto &a64 = x64_to_a64_regs_table[reg.GetCode()].a64;
                if (reg.size == IR::Size::U64) {
                    return a64.X();
                } else if (reg.size == IR::Size::U32) {
                    return a64.W();
                } else {
                    UNREACHABLE();
                }
                break;
            }
            default:
                UNREACHABLE();
        }
    }

    Optional<u8> RegisterManager::AllocInRegList(Vector<Map<u16, u16>> &regs, u16 define, u16 end) {
        for (auto index = 0; index < regs.size(); ++index) {
            auto &intervals = regs[index];
            if (intervals.empty()) {
                intervals.emplace(define, end);
                return index;
            } else {
                auto last = std::prev(intervals.end());
                if (define > last->second) {
                    intervals.emplace(define, end);
                    return index;
                }
            }
        }
        return {};
    }

    const Register &RegisterManager::AcquireTmpX() {
        s8 index = -1;
        for (u8 i = reg_live_interval.size() - 1; i > 0; --i) {
            if (general_register_in_use[i]) {
                continue;
            }
            auto &intervals = reg_live_interval[i];
            if (intervals.empty()) {
                index = i;
                break;
            } else {
                auto itr = intervals.lower_bound(current_ir_id);
                // left of define -> end
                if (itr == intervals.end()) {
                    index = i;
                    break;
                }
                // right of define -> end
                if (itr->second < current_ir_id) {
                    index = i;
                    break;
                }
            }
        }
        ASSERT(index > 0);
        general_register_in_use[index] = true;
        switch (arch) {
            case CpuArch::X64:
                return x64_guest_tmps[index];
            default:
                UNREACHABLE();
        }
    }

    const VRegister &RegisterManager::AcquireTmpV() {
        s8 index = -1;
        for (u8 i = reg_live_interval.size() - 1; i > 0; --i) {
            if (vector_register_in_use[i]) {
                continue;
            }
            auto &intervals = reg_live_interval[i];
            if (intervals.empty()) {
                index = i;
                break;
            } else {
                auto itr = intervals.lower_bound(current_ir_id);
                // left of define -> end
                if (itr == intervals.end()) {
                    index = i;
                    break;
                }
                // right of define -> end
                if (itr->second < current_ir_id) {
                    index = i;
                    break;
                }
            }
        }
        ASSERT(index > 0);
        vector_register_in_use[index] = true;
        switch (arch) {
            case CpuArch::X64:
                return x64_guest_vtmps[index];
            default:
                UNREACHABLE();
        }
    }

    const Register &RegisterManager::Context() {
        return ctx;
    }

    const Register &RegisterManager::PageTale() {
        return page_ptr;
    }

    const Register &RegisterManager::IP() {
        return ip;
    }

    const Register &RegisterManager::Status() {
        return status;
    }

    void RegisterManager::ReleaseTmpX(const Register &r) {
        switch (arch) {
            case CpuArch::X64:
                for (int i = 0; i < x64_guest_tmps.size(); ++i) {
                    if (r == x64_guest_tmps[i]) {
                        general_register_in_use[i] = false;
                        break;
                    }
                }
                return ;
            default:
                UNREACHABLE();
        }
    }

    void RegisterManager::ReleaseTmpV(const VRegister &r) {
        switch (arch) {
            case CpuArch::X64:
                for (int i = 0; i < x64_guest_vtmps.size(); ++i) {
                    if (r == x64_guest_vtmps[i]) {
                        vector_register_in_use[i] = false;
                        break;
                    }
                }
                return ;
            default:
                UNREACHABLE();
        }
    }

    u32 RegisterManager::OffsetOf(const IR::Reg &reg) {
        switch (arch) {
            case CpuArch::X64: {
                auto offset = reg.code * sizeof(X86::Reg);
                // high
                if (reg.type == 1) {
                    offset += reg.SizeByte();
                }
                return offset;
            }
            default:
                UNREACHABLE();
        }
    }

    u32 RegisterManager::OffsetOf(const IR::VReg &reg) {
        switch (arch) {
            case CpuArch::X64: {
                auto offset = OFFSET_OF(X86::ThreadContext64, xmms) + reg.code * sizeof(X86::Xmm);
                // high
                if (reg.type == 1) {
                    offset += reg.SizeByte();
                }
                return offset;
            }
            default:
                UNREACHABLE();
        }
    }

    u32 RegisterManager::PCOffset() {
        switch (arch) {
            case CpuArch::X64: {
                return OFFSET_OF(X86::ThreadContext64, pc);
            }
            default:
                UNREACHABLE();
        }
    }

    void RegisterManager::MarkDirectSetHostReg(u32 instr_id, IR::Reg &reg) {
        const auto &host = ToHostRegister(reg);
        registers_allocated.emplace(instr_id, host);
    }

    void RegisterManager::MarkDirectSetHostReg(u32 instr_id, IR::VReg &reg) {
        const auto &host = ToHostVRegister(reg);
        vregisters_allocated.emplace(instr_id, host);
    }

    void RegisterManager::MarkDirectGetHostReg(u32 instr_id, IR::Reg &reg) {
        const auto &host = ToHostRegister(reg);
        registers_allocated.emplace(instr_id, host);
    }

    void RegisterManager::MarkDirectGetHostReg(u32 instr_id, IR::VReg &reg) {
        const auto &host = ToHostVRegister(reg);
        vregisters_allocated.emplace(instr_id, host);
    }
}
