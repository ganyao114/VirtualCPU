//
// Created by swift on 2021/5/20.
//

#pragma once

#include <ir/block.h>
#include "jit_runtime.h"

namespace Svm {

    SharedPtr<IR::IRBlock> GenerateBlock(JitRuntime *runtime, VAddr block_pc, CpuArch arch);

    SharedPtr<IR::IRBlock> GenerateBlockX64(JitRuntime *runtime, VAddr block_pc);

    bool TranslateBlock(JitRuntime *runtime, IR::IRBlock *block);

}
