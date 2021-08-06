//
// Created by swift on 2021/5/26.
//

#pragma once

#include "types.h"
#include "mutex"
#include "configs.h"
#include "memory/page_entry.h"
#include "base/array_ref.h"

namespace Svm {

    class JitRuntime;
    class VCpu;

    using namespace Memory;

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
