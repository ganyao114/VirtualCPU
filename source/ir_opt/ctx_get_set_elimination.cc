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
        for (auto &instr : instr_seq) {
            if (!instr.Enabled() || ShouldSkip(&instr)) {
                continue;
            }
            switch (instr.GetOpCode()) {
                case OpCode::SetReg: {
                    auto &reg = instr.GetParam<Reg>(0);
                    if (host_regs->CastHostReg(reg)) {
                        auto &value = instr.GetParam<Value>(1);
                        auto direct_access = host_regs->DirectSetHostReg(value.Def());
                        if (direct_access) {
                            instr.Disable();
                            host_regs->MarkDirectSetHostReg(value.Def(), reg);
                        }
                    }
                    break;
                }
                case OpCode::SetVReg: {
                    auto &reg = instr.GetParam<VReg>(0);
                    if (host_regs->CastHostReg(reg)) {
                        auto &value = instr.GetParam<Value>(1);
                        auto direct_access = host_regs->DirectSetHostReg(value.Def());
                        if (direct_access) {
                            instr.Disable();
                            host_regs->MarkDirectSetHostReg(value.Def(), reg);
                        }
                    }
                    break;
                }
                case OpCode::GetReg: {
                    auto &reg = instr.GetParam<Reg>(0);
                    if (host_regs->CastHostReg(reg)) {
                        bool can_get_disable{true};
                        for (auto use : instr.GetUses()) {
                            auto index = use->GetIndex(instr.GetReturn().Get<Value>());
                            if (!host_regs->DirectGetHostReg(use, *index)) {
                                can_get_disable = false;
                            }
                        }
                        if (can_get_disable) {
                            instr.Disable();
                            host_regs->MarkDirectGetHostReg(&instr, reg);
                        }
                    }
                    break;
                }
                case OpCode::GetVReg: {
                    auto &reg = instr.GetParam<VReg>(0);
                    if (host_regs->CastHostReg(reg)) {
                        bool can_get_disable{true};
                        for (auto use : instr.GetUses()) {
                            auto index = use->GetIndex(instr.GetReturn().Get<Value>());
                            if (!host_regs->DirectGetHostReg(use, *index)) {
                                can_get_disable = false;
                            }
                        }
                        if (can_get_disable) {
                            instr.Disable();
                            host_regs->MarkDirectGetHostReg(&instr, reg);
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
        for (auto &instr : instr_seq) {
            switch (instr.GetOpCode()) {
                case OpCode::SetReg: {
                    auto &reg = instr.GetParam<Reg>(0);
                    if (host_regs->CastHostReg(reg)) {
                        auto &value = instr.GetParam<Value>(1);
                        SetValue(reg, value.Def());
                        auto value_from = value.Def();
                        if (value_from->GetUses().size() != 1) {
                            skip_instructions.emplace(&instr);
                        }
                    }
                    break;
                }
                case OpCode::SetVReg: {
                    auto &reg = instr.GetParam<VReg>(0);
                    if (host_regs->CastHostReg(reg)) {
                        auto &value = instr.GetParam<Value>(1);
                        SetValue(reg, &instr);
                        auto value_from = value.Def();
                        if (value_from->GetUses().size() != 1) {
                            skip_instructions.emplace(&instr);
                        }
                    }
                    break;
                }
                case OpCode::GetReg: {
                    auto &reg = instr.GetParam<Reg>(0);
                    if (host_regs->CastHostReg(reg)) {
                        host_regs_versions[&instr] = GetValue(reg);
                    }
                    break;
                }
                case OpCode::GetVReg: {
                    auto &reg = instr.GetParam<VReg>(0);
                    if (host_regs->CastHostReg(reg)) {
                        host_regs_versions[&instr] = GetValue(reg);
                    }
                    break;
                }
                default: {
                    for (u8 i = 0; i < MAX_OPERANDS; ++i) {
                        auto &op = instr.GetOperand(i);
                        if (op.IsValue()) {
                            auto value_from = op.Get<Value>().Def();
                            if (value_from->GetOpCode() == OpCode::GetReg) {
                                // check if use old host register version
                                auto cur_value = GetValue(value_from->GetParam<Reg>(0));
                                if (host_regs_versions[value_from] != cur_value) {
                                    skip_instructions.emplace(value_from);
                                    skip_instructions.emplace(&instr);
                                }
                            } else if (value_from->GetOpCode() == OpCode::GetVReg) {
                                // check if use old host register version
                                auto cur_value = GetValue(value_from->GetParam<VReg>(0));
                                if (host_regs_versions[value_from] != cur_value) {
                                    skip_instructions.emplace(value_from);
                                    skip_instructions.emplace(&instr);
                                }
                            }
                        } else if (op.IsAddress()) {
                            auto &address = op.Get<Address>();
                            if (!address.IsConst()) {
                                auto value_from = address.ValueAddress().Def();
                                if (value_from->GetOpCode() == OpCode::GetReg) {
                                    // check if use old host register version
                                    auto cur_value = GetValue(value_from->GetParam<Reg>(0));
                                    if (host_regs_versions[value_from] != cur_value) {
                                        skip_instructions.emplace(value_from);
                                        skip_instructions.emplace(&instr);
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

    bool CtxGetSetElimination::ShouldSkip(Instruction* inst) {
        return skip_instructions.find(inst) != skip_instructions.end();
    }

}
