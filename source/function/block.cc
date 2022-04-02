//
// Created by swift on 1/8/21.
//

#include "block.h"

namespace Svm {

    Block::Block(ObjectPool<IR::Instruction> *pool, VAddr pc) : ir_block{pc, pool} {}

    void Block::AddPredecessor(Block *block) {
        predecessors.push_back(block);
        block->successors.push_back(this);
    }

    void Block::AddSuccessor(Block *block) {
        successors.push_back(block);
        block->predecessors.push_back(this);
    }

}