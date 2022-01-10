//
// Created by swift on 1/11/21.
//

#include "flags_get_set_sync.h"

// Sync some flags between guest and host
//
// EG:
// Arm NZCV <-> X86 OCSZ


namespace Svm::IR {

    void FlagsSyncOpt::Optimize(IRBlock *block, OptResult *result) {
        auto &instr_seq = block->Sequence();
        auto opt = result->GetOptFlagsGetSet();
        IR::Flags flags_not_all_sync_set{};
        // Opt Set
        for (auto &instr : instr_seq) {
            if (!instr.Enabled()) {
                continue;
            }

            if (instr.GetOpCode() == OpCode::SetFlag) {
                auto flag = instr.GetParam<Flags>(0);
                auto &flag_value = instr.GetParam<Value>(1);
                auto &flag_get = *flag_value.Def();
                Instruction *flag_from{};
                switch (flag_get.GetOpCode()) {
                    case OpCode::GetOverFlow:
                    case OpCode::GetCarry:
                    case OpCode::GetSigned:
                    case OpCode::GetZero:
                        flag_from = flag_get.GetParam<Value>(0).Def();
                        break;
                    case OpCode::GetFlag:
                        flag_from = flag_get.GetParam<Value>(0).Def();
                        break;
                }
                if (flag_from) {
                    auto value_from = flag_from;
                    if (opt->CanSyncFlagSet(flag, value_from)) {
                        opt->SyncFlagSet(value_from, flag);
                        if (flag_get.GetUses().size() == 1) {
                            flag_get.Disable();
                        }
                        instr.Disable();
                    } else {
                        flags_not_all_sync_set.flag |= flag.flag;
                    }
                }
            }
        }

        opt->FlagsCanNotSync(flags_not_all_sync_set);

        // Opt Get Todo
//        for (auto instr : instr_seq) {
//            if (!result->IsEnable(instr->GetId())) {
//                continue;
//            }
//
//            if (instr->GetOpCode() == OpCode::SetFlag) {
//                switch (instr->GetOpCode()) {
//                    case OpCode::BranchCond:
//                        break;
//                    case OpCode::GetOverFlow:
//                        break;
//                    case OpCode::GetCarry:
//                        break;
//                    case OpCode::GetSigned:
//                        break;
//                    case OpCode::GetZero:
//                        break;
//                    case OpCode::GetFlag:
//                        break;
//                }
//            }
//        }
    }

}
