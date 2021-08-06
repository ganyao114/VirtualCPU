//
// Created by SwiftGan on 2021/1/4.
//

#include "ctx_get_set_elimination.h"

// IR SSA -> HOST Non SSA Instr

// X86 Guest:
// MOV R1, R0
//
// IR SSA:
// 1. GetReg: TMP <- R0
// 2. SetReg: TMP -> R1
//
// ARM64 Host:
// MOV X1, X0 (X0 = R0, X1 = R1)

namespace Svm::IR {

    void CtxGetSetElimination::Optimize(IRBlock *block, OptResult *result) {
        auto &instr_seq = block->Sequence();
        auto host_regs = result->GetOptHostReg();
        FilterRegUseForOldVer(block, result, host_regs);
        for (auto instr : instr_seq) {
            if (!result->IsEnable(instr->GetId()) || ShouldSkip(instr->GetId())) {
                continue;
            }
            switch (instr->GetOpCode()) {
                case OpCode::SetReg: {
                    auto &reg = instr->GetParam<Reg>(0);
                    if (host_regs->CastHostReg(reg)) {
                        auto &value = instr->GetParam<Value>(1);
                        auto direct_access = host_regs->DirectSetHostReg(&block->Instr(value.GetId()));
                        if (direct_access) {
                            result->Disable(instr->GetId());
                            host_regs->MarkDirectSetHostReg(value.GetId(), reg);
                        }
                    }
                    break;
                }
                case OpCode::SetVReg: {
                    auto &reg = instr->GetParam<VReg>(0);
                    if (host_regs->CastHostReg(reg)) {
                        auto &value = instr->GetParam<Value>(1);
                        auto direct_access = host_regs->DirectSetHostReg(&block->Instr(value.GetId()));
                        if (direct_access) {
                            result->Disable(instr->GetId());
                            host_regs->MarkDirectSetHostReg(value.GetId(), reg);
                        }
                    }
                    break;
                }
                case OpCode::GetReg: {
                    auto &reg = instr->GetParam<Reg>(0);
                    if (host_regs->CastHostReg(reg)) {
                        bool can_get_disable{true};
                        for (auto use : instr->GetUses()) {
                            auto &use_reg_instr = block->Instr(use);
                            auto index = use_reg_instr.GetIndex(instr->GetReturn().Get<Value>());
                            if (!host_regs->DirectGetHostReg(&use_reg_instr, *index)) {
                                can_get_disable = false;
                            }
                        }
                        if (can_get_disable) {
                            result->Disable(instr->GetId());
                            host_regs->MarkDirectGetHostReg(instr->GetId(), reg);
                        }
                    }
                    break;
                }
                case OpCode::GetVReg: {
                    auto &reg = instr->GetParam<VReg>(0);
                    if (host_regs->CastHostReg(reg)) {
                        bool can_get_disable{true};
                        for (auto use : instr->GetUses()) {
                            auto &use_reg_instr = block->Instr(use);
                            auto index = use_reg_instr.GetIndex(instr->GetReturn().Get<Value>());
                            if (!host_regs->DirectGetHostReg(&use_reg_instr, *index)) {
                                can_get_disable = false;
                            }
                        }
                        if (can_get_disable) {
                            result->Disable(instr->GetId());
                            host_regs->MarkDirectGetHostReg(instr->GetId(), reg);
                        }
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    void CtxGetSetElimination::FilterRegUseForOldVer(IRBlock *block, OptResult *result, OptHostReg *host_regs) {
        auto &instr_seq = block->Sequence();
        for (auto instr : instr_seq) {
            switch (instr->GetOpCode()) {
                case OpCode::SetReg: {
                    auto &reg = instr->GetParam<Reg>(0);
                    if (host_regs->CastHostReg(reg)) {
                        auto &value = instr->GetParam<Value>(1);
                        SetValue(reg, value.GetId());
                        auto &value_from = block->Instr(value.GetId());
                        if (value_from.GetUses().size() != 1) {
                            skip_instructions.emplace(instr->GetId());
                        }
                    }
                    break;
                }
                case OpCode::SetVReg: {
                    auto &reg = instr->GetParam<VReg>(0);
                    if (host_regs->CastHostReg(reg)) {
                        auto &value = instr->GetParam<Value>(1);
                        SetValue(reg, value.GetId());
                        auto &value_from = block->Instr(value.GetId());
                        if (value_from.GetUses().size() != 1) {
                            skip_instructions.emplace(instr->GetId());
                        }
                    }
                    break;
                }
                case OpCode::GetReg: {
                    auto &reg = instr->GetParam<Reg>(0);
                    if (host_regs->CastHostReg(reg)) {
                        host_regs_versions[instr->GetId()] = GetValue(reg);
                    }
                    break;
                }
                case OpCode::GetVReg: {
                    auto &reg = instr->GetParam<VReg>(0);
                    if (host_regs->CastHostReg(reg)) {
                        host_regs_versions[instr->GetId()] = GetValue(reg);
                    }
                    break;
                }
                default: {
                    for (u8 i = 0; i < MAX_OPERANDS; ++i) {
                        auto &op = instr->GetOperand(i);
                        if (op.IsValue()) {
                            auto id = op.Get<Value>().GetId();
                            auto &value_from = block->Instr(id);
                            if (value_from.GetOpCode() == OpCode::GetReg) {
                                // check if use old host register version
                                auto cur_value = GetValue(value_from.GetParam<Reg>(0));
                                if (host_regs_versions[value_from.GetId()] != cur_value) {
                                    skip_instructions.emplace(id);
                                    skip_instructions.emplace(instr->GetId());
                                }
                            } else if (value_from.GetOpCode() == OpCode::GetVReg) {
                                // check if use old host register version
                                auto cur_value = GetValue(value_from.GetParam<VReg>(0));
                                if (host_regs_versions[value_from.GetId()] != cur_value) {
                                    skip_instructions.emplace(id);
                                    skip_instructions.emplace(instr->GetId());
                                }
                            }
                        } else if (op.IsAddress()) {
                            auto &address = op.Get<Address>();
                            if (!address.IsConst()) {
                                auto id = address.ValueAddress().GetId();
                                auto &value_from = block->Instr(id);
                                if (value_from.GetOpCode() == OpCode::GetReg) {
                                    // check if use old host register version
                                    auto cur_value = GetValue(value_from.GetParam<Reg>(0));
                                    if (host_regs_versions[value_from.GetId()] != cur_value) {
                                        skip_instructions.emplace(id);
                                        skip_instructions.emplace(instr->GetId());
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }
    }

    bool CtxGetSetElimination::ShouldSkip(u32 id) {
        return skip_instructions.find(id) != skip_instructions.end();
    }

}
