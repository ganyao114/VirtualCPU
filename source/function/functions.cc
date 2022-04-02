//
// Created by 甘尧 on 2022/1/29.
//

#include <ir/assembler.h>
#include <frontend/x64/decoder.h>
#include "functions.h"

namespace Svm {


    Function::Function(VAddr pc, Runtime *runtime) : start_pc(pc), runtime(runtime), memory(&runtime->GetMemory()) {}

    void Function::Build() {
        auto pc = start_pc;
        std::list<VAddr> basic_block_start{};

        auto find_block = [&] (VAddr vaddr) -> Block* {
            for (auto &block : blocks) {
                if (block.Overlap(vaddr)) {
                    return &block;
                }
            }
            return {};
        };

        // build blocks
        basic_block_start.push_back(pc);
        while (!basic_block_start.empty()) {
            auto block_start = basic_block_start.begin();
            basic_block_start.erase(block_start);
            pc = *block_start;
            if (auto found = find_block(pc)) {
                if (found->StartPC() == pc) {
                    continue;
                } else {
                    auto split_offset = pc - found->StartPC();
                    auto new_ir_block = NewBlock(pc);
                    found->GetIRBlock().Split(&new_ir_block->GetIRBlock(), split_offset);
                    continue;
                }
            }
            auto block = CreateBlock(pc);
            if (auto address = FunctionCall(block); address) {
                near_function_start.emplace(*address);
            }
            if (auto next_blocks = NextBlocks(block); !next_blocks.empty()) {
                for (auto &next : next_blocks) {
                    basic_block_start.push_back(next);
                }
            }
        }

        // build DAG
        for (auto &block : blocks) {
            if (auto next_blocks = NextBlocks(&block); !next_blocks.empty()) {
                for (auto &next : next_blocks) {
                    auto next_block = find_block(next);
                    block.AddSuccessor(next_block);
                }
            }
        }
    }

    void Function::Optimize() {
        using SCC = std::list<u32>;
        std::list<SCC> sccs{};
        auto block_count = blocks.size();
        std::vector<int> dfn(block_count);
        std::vector<int> low(block_count);
        std::stack<int> node_stack{};
        std::vector<bool> in_stack(block_count);
        int index{0};
        std::fill(dfn.begin(), dfn.end(), -1);
        std::fill(low.begin(), low.end(), -1);
        std::fill(in_stack.begin(), in_stack.end(), false);

        auto tarjan = [&] (u32 block_index) {
            std::function<void (u32)> impl;
            impl = [&](u32 block_index) {
                auto block = block_ids[block_index];
                dfn[block_index] = low[block_index] = ++index;
                node_stack.push(block_index);
                in_stack[block_index] = true;

                for (auto scc_block : block->Successors()) {
                    auto scc_index = scc_block->GetId();
                    if (dfn[scc_index] == -1) {
                        impl(scc_index);
                        low[block_index] = std::min(low[block_index], low[scc_index]);
                    } else if (in_stack[scc_index]) {
                        low[block_index] = std::min(low[block_index], dfn[scc_index]);
                    }
                }
                if (low[block_index] == dfn[block_index]) {
                    auto &list = sccs.emplace_back();
                    int current = -1;
                    while (block_index != current) {
                        current = node_stack.top();
                        node_stack.pop();
                        in_stack[current] = false;
                        list.push_back(current);
                    }
                }
            };
            impl(block_index);
        };

        for (auto id = 0; id < blocks.size(); id++) {
            if (dfn[id] < 0) {
                tarjan(id);
            }
        }

        // add trap check
        for (auto &scc : sccs) {
            auto scc_start = *scc.begin();
            auto loop_head_block = block_ids[scc_start];
            if (scc.size() == 1) {
                bool loop_self{false};
                for (auto scc_block : loop_head_block->Successors()) {
                    if (scc_block == loop_head_block) {
                        loop_self = true;
                        break;
                    }
                }
                if (!loop_self) {
                    continue;
                }
            }
            loop_head_block->GetIRBlock().CheckHalt();
        }
    }

    Block *Function::FindBlock(VAddr block_start) {
        for (auto &block : blocks) {
            if (block.StartPC() == block_start) {
                return &block;
            }
        }
        return {};
    }

    Block *Function::CreateBlock(VAddr pc) {
        auto block = NewBlock(pc);
        IR::Assembler ir_asm{block->GetIRBlock()};
        switch (runtime->GuestArch()) {
            case CpuArch::X64: {
                Decoder::X64Decoder decoder{pc, &ir_asm, &runtime->GetMemory()};
                decoder.Decode();
                break;
            }
            default:
                abort();
        }
        return block;
    }

    Block *Function::NewBlock(VAddr pc) {
        auto block = &blocks.emplace_back(&instr_pool, pc);
        block->SetId(blocks.size() - 1);
        block_ids.emplace(block->GetId(), block);
        return block;
    }

    std::list<VAddr> Function::NextBlocks(Block *block) {
        auto &ir = block->GetIRBlock();
        return ir.NextBlocksAddress(false);
    }

    std::optional<VAddr> Function::FunctionCall(Block *block) {
        auto &ir = block->GetIRBlock();
        auto end_with_call = ir.GetTermReason() == ir.FUNC_CALL && ir.GetTermType() == ir.DIRECT;
        if (!end_with_call) {
            return {};
        }
        auto &call_address = ir.TermDirect().next_block;
        if (!call_address.IsConst()) {
            return {};
        }
        return call_address.ConstAddress().Value<VAddr>();
    }
}
