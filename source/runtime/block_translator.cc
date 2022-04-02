//
// Created by swift on 2021/5/20.
//

#include "block_translator.h"
#include <frontend/x64/decoder.h>
#include <backend/arm64/jit_context.h>
#include <backend/arm64/ir_translator.h>
#include <ir_opt/ir_opt_manager.h>

using namespace Svm::A64;

namespace Svm {

    std::shared_ptr<IR::IRBlock> GenerateBlock(JitRuntime *runtime, VAddr block_pc, CpuArch guest_arch) {
        std::shared_ptr<IR::IRBlock> res{};
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

    std::shared_ptr<IR::IRBlock> GenerateBlockX64(JitRuntime *runtime, VAddr block_pc) {
        auto ir_block = runtime->NewIRBlock(block_pc);
        IR::Assembler ir_asm{*ir_block};
        Decoder::X64Decoder decoder{block_pc, &ir_asm, &runtime->GetMemory()};
        decoder.Decode();
        ir_block->CheckHalt();
        return ir_block;
    }

    bool TranslateBlock(JitRuntime *runtime, IR::IRBlock *block) {
        block->PrepareOpt();
        MacroAssembler masm{};
        A64JitContext context{block, runtime, masm};
        IRCommitA64 ir_commit{block, &context};
        IR::OptimizeIR(block, ir_commit.IROptRes());
        ir_commit.Translate();
        context.EndBlock();
        auto cache_size = masm.GetBuffer()->GetSizeInBytes();
        auto[base, cache] = runtime->GetBlockCache(block->StartPC());
        CodeBuffer buffer{};
        if (cache->BoundModule()) {
            auto module = runtime->GetModule(cache->GetModule().module_id);
            buffer = module->AllocBuffer(cache->GetModule().block_id, cache_size);
        } else {
            buffer = runtime->GetCodeCachePool().Alloc(cache_size);
        }
        context.Flush(buffer);
        memcpy(buffer.rw_data, masm.GetBuffer()->GetStartAddress<u8 *>(), cache_size);
        buffer.Flush();
        if (!cache->BoundModule()) {
            runtime->GetDispatcher().GetEntry(
                    cache->GetDispatchIndex()).value = reinterpret_cast<VAddr>(buffer.exec_data);
        }
        return true;
    }

}
