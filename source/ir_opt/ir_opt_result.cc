//
// Created by SwiftGan on 2021/1/4.
//

#include "ir_opt_result.h"

namespace Svm::IR {

    bool OptHostReg::CastHostReg(Reg &reg) {
        return false;
    }

    bool OptHostReg::CastHostReg(VReg &reg) {
        return false;
    }

    bool OptHostReg::DirectSetHostReg(Instruction *instr) {
        return false;
    }

    bool OptHostReg::DirectGetHostReg(Instruction *instr, u8 op_index) {
        return false;
    }

    void OptHostReg::MarkDirectSetHostReg(u32 instr_id, IR::Reg &reg) {
    }

    void OptHostReg::MarkDirectSetHostReg(u32 instr_id, IR::VReg &reg) {
    }

    void OptHostReg::MarkDirectGetHostReg(u32 instr_id, VReg &reg) {
    }

    void OptHostReg::MarkDirectGetHostReg(u32 instr_id, IR::Reg &reg) {
    }

    bool OptValueFold::CouldFold(Instruction *dest, Instruction *src) {
        return false;
    }

    void OptValueFold::MarkFold(u32 value_src_id, Set<u32> &dest_instr_set) {
    }

    OptValueFold::Op *OptValueFold::GetFoldOperand(u32 value_src_id) {
        return {};
    }

    bool OptConstRead::IsReadOnly(VAddr addr) {
        return false;
    }

    Vector<u8> OptConstRead::ReadMemory(VAddr addr, size_t size) {
        return Svm::Vector<u8>();
    }

    bool OptFlagsGetSet::CanSyncFlagSet(Flags &flag, Instruction *instr_flag_from) {
        return false;
    }

    bool OptFlagsGetSet::CanSyncFlagGet(Flags &flag, Instruction *instr_flag_consume) {
        return false;
    }

    void OptFlagsGetSet::SyncFlagSet(Instruction *instr_flag_from, Flags &flag) {

    }

    void OptFlagsGetSet::SyncFlagGet(Instruction *instr_flag_consume, Flags &flag) {

    }

    void OptFlagsGetSet::FlagsCanNotSync(Flags &flag) {

    }

    void OptResult::Disable(u32 id) {
        disables.emplace(id);
    }

    void OptResult::Enable(u32 id) {
        disables.erase(id);
    }

    OptHostReg *OptResult::GetOptHostReg() {
        return nullptr;
    }

    OptValueFold *OptResult::GetOptValueFold() {
        return nullptr;
    }

    OptConstRead *OptResult::GetOptConstRead() {
        return nullptr;
    }

    OptFlagsGetSet *OptResult::GetOptFlagsGetSet() {
        return nullptr;
    }

    RegAllocPass *OptResult::GetRegAllocPass() {
        return nullptr;
    }
}
