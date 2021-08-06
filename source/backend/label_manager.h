//
// Created by swift on 1/8/21.
//

#pragma once

#include <base/marco.h>

namespace Svm {

    template <typename Label>
    class BaseLabelManager : public BaseObject, CopyDisable {
    public:
        
        Label *AllocLabel() {
            auto res = &labels.emplace_back();
            return res;
        }
        
    private:
        List<Label> labels;
    };

}
