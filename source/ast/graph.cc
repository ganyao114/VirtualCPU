//
// Created by swift on 1/8/21.
//

#include "graph.h"

namespace Svm::Ast {

    Graph::Graph(VAddr pc) : start_pc(pc) {}

    void Graph::AddBlock(const SharedPtr<BasicBlock> &block) {
        blocks.push_back(block);
    }

}