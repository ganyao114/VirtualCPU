//
// Created by swift on 1/8/21.
//

#pragma once

#include <base/marco.h>
#include "block.h"

namespace Svm::Ast {

    class Graph : public BaseObject, CopyDisable {
    public:

        explicit Graph(VAddr pc);

        void AddBlock(const SharedPtr<BasicBlock> &block);
        
        constexpr void SetEntry(BasicBlock* block) {
            entry_block = block;
        }
        
        constexpr void SetExit(BasicBlock* block) {
            exit_block = block;
        }
        
        constexpr List<SharedPtr<BasicBlock>> &Blocks() {
            return blocks;
        }

    private:
        VAddr start_pc;
        List<SharedPtr<BasicBlock>> blocks;
        BasicBlock* entry_block;
        BasicBlock* exit_block;
    };

}
