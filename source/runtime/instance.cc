//
// Created by swift on 2021/6/29.
//

#include <include/svm_instance.h>
#include "jit_runtime.h"

namespace Svm {

    SvmInstance::SvmInstance(UserConfigs &configs) {
        runtime = std::make_shared<JitRuntime>(configs);
    }

    ArrayRef<PageEntry> SvmInstance::PageTable() {
        auto &page_table = runtime->GetPageTable();
        return {page_table.PageTablePtr(), page_table.PageEntries()};
    }

}

