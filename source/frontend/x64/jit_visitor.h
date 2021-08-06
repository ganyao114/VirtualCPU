//
// Created by SwiftGan on 2021/1/1.
//

#pragma once

#include <frontend/visitor.h>
#include <ir/assembler.h>

namespace Svm::Decoder {

    class X64JitVisitor : public Visitor, public IR::Assembler {
    public:
    };

}
