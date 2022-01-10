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

    void OptHostReg::MarkDirectSetHostReg(Instruction *instr, IR::Reg &reg) {
    }

    void OptHostReg::MarkDirectSetHostReg(Instruction *instr, IR::VReg &reg) {
    }

    void OptHostReg::MarkDirectGetHostReg(Instruction *instr, VReg &reg) {
    }

    void OptHostReg::MarkDirectGetHostReg(Instruction *instr, IR::Reg &reg) {
    }

    bool OptValueFold::CouldFold(Instruction *dest, Instruction *src) {
        return false;
    }

    void OptValueFold::MarkFold(Instruction *value_src, Set<Instruction*> &dest_instr_set) {
    }

    OptValueFold::Op *OptValueFold::GetFoldOperand(Instruction *value_src) {
        return {};
    }

    bool OptConstRead::IsReadOnly(VAddr addr) {
        return false;
    }

    Vector<u8> OptConstRead::ReadMemory(VAddr addr, size_t size) {
        return Svm::Vector<u8>();
    }

    bool OptFlagsSync::CanSyncFlagSet(Flags &flag, Instruction *instr_flag_from) {
        return false;
    }

    bool OptFlagsSync::CanSyncFlagGet(Flags &flag, Instruction *instr_flag_consume) {
        return false;
    }

    void OptFlagsSync::SyncFlagSet(Instruction *instr_flag_from, Flags &flag) {

    }

    void OptFlagsSync::SyncFlagGet(Instruction *instr_flag_consume, Flags &flag) {

    }

    void OptFlagsSync::FlagsCanNotSync(Flags &flag) {

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

    OptFlagsSync *OptResult::GetOptFlagsGetSet() {
        return nullptr;
    }

    RegAllocPass *OptResult::GetRegAllocPass() {
        return nullptr;
    }
}
