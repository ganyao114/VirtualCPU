//
// Created by swift on 1/9/21.
//

#pragma once

#include <base/marco.h>
#include <ast/block.h>
#include <include/memory_interface.h>

namespace Svm {

    using namespace Memory;

    SharedPtr<Ast::Graph> BuildFunction(VAddr pc, MemoryInterface<VAddr> *memory);

    SharedPtr<Ast::BasicBlock> DecodeBasicBlock(Ast::Graph *graph, VAddr pc, MemoryInterface<VAddr> *memory);

}
