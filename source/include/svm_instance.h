//
// Created by swift on 2021/5/26.
//

#pragma once

#include "types.h"
#include "mutex"
#include "configs.h"
#include "page_entry.h"
#include "base/array_ref.h"
#include "memory_interface.h"

namespace Svm {

    class JitRuntime;
    class VCpu;

    using namespace Memory;

    class Runtime {
    public:
        virtual UserConfigs *GetConfigs() = 0;

        virtual Memory::PageTableConst *GetPageTableConst() = 0;

        virtual bool Static() = 0;

        virtual CpuArch GuestArch() = 0;

        virtual MemoryInterface &GetMemory() = 0;

        virtual void PutCodeCache(VAddr pc, PAddr cache) = 0;

        virtual PAddr GetCodeCache(VAddr pc) = 0;

        virtual PAddr FlushCodeCache(const u8 *buffer, size_t size) = 0;

        virtual void PushLinkPoint(VAddr target, PAddr link_point) = 0;
    };

    class SvmInstance : CopyDisable {
    public:

        explicit SvmInstance(UserConfigs &configs);

        ArrayRef<PageEntry> PageTable();

        constexpr JitRuntime &Runtime() {
            return *runtime;
        }

    private:
        std::shared_ptr<JitRuntime> runtime;
    };

}
