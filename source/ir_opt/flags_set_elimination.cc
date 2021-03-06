//
// Created by swift on 2021/8/30.
//

#include "flags_set_elimination.h"

namespace Svm::IR {

    void FlagsSetOpt::Optimize(IRBlock *block, OptResult *result) {
        std::map<u32, std::set<u32>> flags_set_positions{};
        std::map<u32, std::set<u32>> flags_get_positions{};

        auto append_flag_sets = [&flags_set_positions] (u32 flag, u32 instr_id) {
            flags_set_positions[flag].emplace(instr_id);
        };

        auto append_flag_gets = [&flags_get_positions] (u32 flag, u32 instr_id) {
            flags_get_positions[flag].emplace(instr_id);
        };

        auto &instr_seq = block->Sequence();
        // Opt Set
        for (auto &instr : instr_seq) {
            if (instr.GetOpCode() == OpCode::SetFlag) {
                auto flag = instr.GetParam<Flags>(0).flag;
                append_flag_sets(flag, instr.GetId());
            } else if (instr.GetOpCode() == OpCode::GetFlag) {
                auto flag = instr.GetParam<Flags>(1).flag;
                append_flag_gets(flag, instr.GetId());
            } else if (instr.GetOpCode() == OpCode::ClearFlags) {
                auto flags = instr.GetParam<Flags>(0).flag;
                for (int i = 0; i < sizeof(Flags::FlagValue) * 8; ++i) {
                    if ((flags >> i) & 1) {
                        append_flag_sets(Flags::FlagValue(1) << i, instr.GetId());
                    }
                }
            }
        }

        for (auto &[flag, set_positions] : flags_set_positions) {
            auto itr = flags_get_positions.find(flag);
            std::set<u32> keep_sets{};
            if (itr != flags_get_positions.end()) {
                auto &get_positions = itr->second;
                for (auto get_position : get_positions) {
                    auto it = set_positions.lower_bound(get_position);
                    if (it == set_positions.begin()) {
                        continue;
                    }
                    it = std::prev(it);
                    if (it != set_positions.end() && *it <= get_position) {
                        // add to keep sets
                        keep_sets.emplace(*it);
                        set_positions.erase(it);
                    }
                }
            }
            // we must keep last flag set inst
            if (keep_sets.empty()) {
                auto it = std::prev(set_positions.end());
                keep_sets.emplace(*it);
                set_positions.erase(it);
            }
            // now remove useless set inst
            for (auto set_inst_id : set_positions) {
                auto &instr = block->Instr(set_inst_id);
                if (instr.GetOpCode() == OpCode::ClearFlags) {
                    auto &flags = instr.GetParam<Flags>(0).flag;
                    // clear flag
                    flags &= ~flag;
                    if (flags) {
                        block->Instr(set_inst_id).Disable();
                    }
                } else {
                    block->Instr(set_inst_id).Disable();
                }
            }
        }
    }

}
