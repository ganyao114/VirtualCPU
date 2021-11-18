//
// Created by swift on 2021/8/30.
//

#include "flags_set_elimination.h"

namespace Svm::IR {

    void FlagsSetOpt::Optimize(IRBlock *block, OptResult *result) {
        Map<u32, Set<u32>> flags_set_positions{};
        Map<u32, Set<u32>> flags_get_positions{};

        auto append_flag_sets = [&flags_set_positions] (u32 flag, u32 instr_id) {
            flags_set_positions[flag].emplace(instr_id);
        };

        auto append_flag_gets = [&flags_get_positions] (u32 flag, u32 instr_id) {
            flags_get_positions[flag].emplace(instr_id);
        };

        auto &instr_seq = block->Sequence();
        // Opt Set
        for (auto instr : instr_seq) {
            if (instr->GetOpCode() == OpCode::SetFlag) {
                auto flag = instr->GetParam<Flags>(0).flag;
                append_flag_sets(flag, instr->GetId());
            } else if (instr->GetOpCode() == OpCode::GetFlag) {
                auto flag = instr->GetParam<Flags>(1).flag;
                append_flag_gets(flag, instr->GetId());
            }
        }

        for (auto &[flag, set_positions] : flags_set_positions) {
            auto itr = flags_get_positions.find(flag);
            Set<u32> keep_sets{};
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
                result->Disable(set_inst_id);
            }
        }
    }

}
