//
// Created by swift on 2021/5/20.
//

#include "basic_block.h"
#include <frontend/x64/decoder.h>
#include <backend/arm64/jit_context.h>
#include <backend/arm64/ir_translator.h>
#include <ir_opt/ir_opt_manager.h>

using namespace Svm::A64;

namespace Svm {

    SharedPtr<IR::IRBlock> GenerateBlock(JitRuntime *runtime, VAddr block_pc, CpuArch guest_arch) {
        SharedPtr<IR::IRBlock> res{};
        switch (guest_arch) {
            case CpuArch::X64:
                res = GenerateBlockX64(runtime, block_pc);
                break;
            default:
                break;
        }
        if (res) {
            res->EndBlock();
        }
        return res;
    }

    SharedPtr<IR::IRBlock> GenerateBlockX64(JitRuntime *runtime, VAddr block_pc) {
        auto ir_block = runtime->NewIRBlock(block_pc);
        IR::Assembler ir_asm{*ir_block};
        Decoder::X64Decoder decoder{block_pc, &ir_asm, &runtime->GetMemory64()};
        decoder.Decode();
        ir_asm.CheckHalt();
        return ir_block;
    }

    bool TranslateBlock(JitRuntime *runtime, IR::IRBlock *block) {
        block->PrepareOpt();
        A64JitContext context{block, runtime};
        IRCommitA64 ir_commit{block, &context};
        IR::OptimizeIR(block, ir_commit.IROptRes());
        ir_commit.Translate();
        ir_commit.Flush();
        return true;
    }

}
