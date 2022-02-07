//
// Created by 甘尧 on 2022/1/27.
//

#pragma once

#include "include/types.h"
#include "aarch64/macro-assembler-aarch64.h"

namespace Svm {

    using namespace vixl::aarch64;

    bool LinkBlock(PAddr source, PAddr target, u8 *source_rw, bool pic = true);

}
