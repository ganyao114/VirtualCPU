//
// Created by SwiftGan on 2020/12/29.
//

#pragma once

#include <base/marco.h>
#include "visitor.h"
#include <ir/assembler.h>

namespace Svm::Decoder {

    class BaseDecoder : public BaseObject, CopyDisable {
    public:

        virtual void Decode() = 0;

    };

}
