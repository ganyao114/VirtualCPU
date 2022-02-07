//
// Created by swift on 1/8/21.
//

#pragma once

#include "ir_opt_result.h"
#include "optimize.h"
#include <runtime/jit_runtime.h>

namespace Svm::IR {

    class OptConstReadImpl : public OptConstRead {
    public:

        explicit OptConstReadImpl(BasePageTable *memory);

        bool IsReadOnly(VAddr addr) override;

        Vector<u8> ReadMemory(VAddr addr, size_t size) override;

    private:
        BasePageTable *memory{};
    };

    class ConstMemoryReadOpt : public IROptimize {
    public:

        void Optimize(IRBlock *block, OptResult *result) override;

    private:

        void TransferToRead(Instruction *instr, VAddr vaddr, OptConstRead *opt, Size size);

    };

}