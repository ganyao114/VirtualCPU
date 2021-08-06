//
// Created by swift on 1/11/21.
//

#include "flags_get_set_elimination.h"

// Sync some flags between guest and host
//
// EG:
// Arm NZCV <-> X86 OCSZ


namespace Svm::IR {

    void FlagsGetSetOpt::Optimize(IRBlock *block, OptResult *result) {
        auto &instr_seq = block->Sequence();
        auto opt = result->GetOptFlagsGetSet();
        IR::Flags flags_not_all_sync_set{};
        // Opt Set
        for (auto instr : instr_seq) {
            if (!result->IsEnable(instr->GetId())) {
                continue;
            }

            if (instr->GetOpCode() == OpCode::SetFlag) {
                auto flag = instr->GetParam<Flags>(0);
                auto &flag_value = instr->GetParam<Value>(1);
                auto &flag_get = block->Instr(flag_value.GetId());
                u32 flag_from_id{};
                switch (flag_get.GetOpCode()) {
                    case OpCode::GetOverFlow:
                    case OpCode::GetCarry:
                    case OpCode::GetSigned:
                    case OpCode::GetZero:
                        flag_from_id = flag_get.GetParam<Value>(0).GetId();
                        break;
                    case OpCode::GetFlag:
                        flag_from_id = flag_get.GetParam<Value>(0).GetId();
                        break;
                }
                if (flag_from_id) {
                    auto value_from = &block->Instr(flag_from_id);
                    if (opt->CanSyncFlagSet(flag, value_from)) {
                        opt->SyncFlagSet(value_from, flag);
                        if (flag_get.GetUses().size() == 1) {
                            result->Disable(flag_get.GetId());
                        }
                        result->Disable(instr->GetId());
                    } else {
                        flags_not_all_sync_set.index |= flag.index;
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
