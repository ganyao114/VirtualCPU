//
// Created by swift on 1/8/21.
//

#pragma once

#include <base/marco.h>
#include <ir/block.h>

namespace Svm::Ast {

    class Graph;

    class BasicBlock : public BaseObject, CopyDisable {
    public:

        explicit BasicBlock(Graph *graph, VAddr pc, u32 size);

        void AddPredecessor(BasicBlock* block);

        void AddSuccessor(BasicBlock* block);

        constexpr Vector<BasicBlock*> &Predecessors() {
            return predecessors;
        }

        constexpr Vector<BasicBlock*> &Successors() {
            return predecessors;
        }
        
        constexpr void SetSize(u32 size) {
            this->size = size;
        }

        constexpr void SetIRBlock(const SharedPtr<IR::IRBlock> &block) {
            ir_block = block;
        }

        constexpr SharedPtr<IR::IRBlock> &GetIRBlock() {
            return ir_block;
        }
        
        constexpr VAddr StartPC() const {
            return block_start;
        }
        
        constexpr bool InBlock(VAddr pc) const {
            return pc >= block_start && pc < (block_start + size);
        }

    private:
        Graph *graph;
        VAddr block_start;
        u32 size;
        Vector<BasicBlock*> predecessors;
        Vector<BasicBlock*> successors;
        BasicBlock* dominator;
        List<BasicBlock*> dominated_blocks;
        SharedPtr<IR::IRBlock> ir_block;
    };

}
