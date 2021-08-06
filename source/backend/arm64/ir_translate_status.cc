//
// Created by swift on 2021/6/25.
//

#include "ir_translator.h"

#define __ context->Masm().

namespace Svm::A64 {

    void IRCommitA64::GetCarry(IR::Value &a1) {

    }

    void IRCommitA64::GetFlag(IR::Value &a1, IR::Flags &a2) {

    }

    void IRCommitA64::SetFlag(IR::Flags &a1, IR::Value &a2) {

    }

    void IRCommitA64::MarkFlag(IR::Flags &a1) {

    }

    void IRCommitA64::GetOverFlow(IR::Value &a1) {

    }

    void IRCommitA64::GetSigned(IR::Value &a1) {

    }

    void IRCommitA64::GetZero(IR::Value &a1) {

    }

    void IRCommitA64::ClearFlags(IR::Flags &a1) {

    }

    void IRCommitA64::CheckFlag(IR::Flags &a1) {

    }

    void IRCommitA64::CheckCond(IR::Cond &a1) {

    }

    bool IRCommitA64::CanSyncFlagSet(IR::Flags &flag, IR::Instruction *instr_flag_from) {
        return OptFlagsGetSet::CanSyncFlagSet(flag, instr_flag_from);
    }

    bool IRCommitA64::CanSyncFlagGet(IR::Flags &flag, IR::Instruction *instr_flag_consume) {
        return OptFlagsGetSet::CanSyncFlagGet(flag, instr_flag_consume);
    }

    void IRCommitA64::SyncFlagSet(IR::Instruction *instr_flag_from, IR::Flags &flag) {
        OptFlagsGetSet::SyncFlagSet(instr_flag_from, flag);
    }

    void IRCommitA64::SyncFlagGet(IR::Instruction *instr_flag_consume, IR::Flags &flag) {
        OptFlagsGetSet::SyncFlagGet(instr_flag_consume, flag);
    }

    void IRCommitA64::FlagsCanNotSync(IR::Flags &flag) {
        OptFlagsGetSet::FlagsCanNotSync(flag);
    }

}