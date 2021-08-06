//
// Created by swift on 1/8/21.
//

#include "block.h"
#include "graph.h"

namespace Svm::Ast {

    BasicBlock::BasicBlock(Graph *graph, VAddr pc, u32 size) : graph(graph), block_start(pc), size(size) {}

    void BasicBlock::AddPredecessor(BasicBlock *block) {
        predecessors.push_back(block);
        block->successors.push_back(this);
    }

    void BasicBlock::AddSuccessor(BasicBlock *block) {
        successors.push_back(block);
        block->predecessors.push_back(this);
    }

}