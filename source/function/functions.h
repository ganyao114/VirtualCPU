//
// Created by 甘尧 on 2022/1/29.
//

#pragma once

#include "include/memory_interface.h"
#include "include/svm_instance.h"
#include "block.h"

namespace Svm {

    class Function {
    public:

        explicit Function(VAddr pc, Runtime *runtime);

        void Build();

        void Optimize();

        Block *FindBlock(VAddr block_start);

        constexpr std::list<Block> &Blocks() {
            return blocks;
        }

        [[nodiscard]] constexpr VAddr StartPC() const {
            return start_pc;
        }

        constexpr std::set<VAddr> &NearFunctions() {
            return near_function_start;
        }

    private:

        Block *CreateBlock(VAddr pc);

        Block *NewBlock(VAddr pc);

        std::list<VAddr> NextBlocks(Block *block);

        std::optional<VAddr> FunctionCall(Block *block);

        VAddr start_pc;
        Runtime *runtime;
        Memory::MemoryInterface *memory;
        // instr pool must destroy after blocks
        ObjectPool<IR::Instruction> instr_pool{};
        std::list<Block> blocks;
        std::set<VAddr> near_function_start{};
        std::map<u32, Block*> block_ids{};
    };

}