//
// Created by SwiftGan on 2020/12/29.
//

#pragma once

#include <base/marco.h>

namespace Svm::Decoder {

    class Visitor : public BaseObject, CopyDisable {
    public:

        constexpr void Terminal() {
            terminal = true;
        }

        constexpr bool Termed() {
            return terminal;
        }

    private:
        bool terminal{false};
    };

}
