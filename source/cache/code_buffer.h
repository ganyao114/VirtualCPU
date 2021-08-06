//
// Created by swift on 2021/6/22.
//

#pragma once

#include <base/cow_vector.h>

namespace Svm::Cache {

    struct CodeBuffer {
        constexpr CodeBuffer() {};

        constexpr CodeBuffer(u8 *exec, u8 *rw, u32 size) : exec_data(exec), rw_data(rw), size(size) {}

        constexpr CodeBuffer(u8 *exec, u8 *stub_data, u8 *rw, u32 size) : exec_data(exec), stub_data(stub_data),
                                                                          rw_data(rw), size(size) {}

        void Flush() const {
            Platform::ClearDCache(reinterpret_cast<VAddr>(rw_data), size);
            Platform::ClearDCache(reinterpret_cast<VAddr>(exec_data), size);
            Platform::ClearICache(reinterpret_cast<VAddr>(exec_data), size);
        }

        u8 *exec_data{};
        u8 *stub_data{};
        u8 *rw_data{};
        u32 size{};
    };

}
