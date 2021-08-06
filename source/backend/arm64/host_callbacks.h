//
// Created by swift on 2021/5/25.
//

#pragma once

#include "backend/cpu.h"

namespace Svm::A64 {

    struct HostCallbacks {
        VAddr (*ReadMemory)(VAddr va, u8 size);
        void  (*WriteMemory)(VAddr va, u64 value, u8 size);
        u8    (*Read8)(VAddr va);
        u16   (*Read16)(VAddr va);
        u32   (*Read32)(VAddr va);
        u64   (*Read64)(VAddr va);
        void  (*Write8)(VAddr va, u8 value);
        void  (*Write16)(VAddr va, u16 value);
        void  (*Write32)(VAddr va, u32 value);
        void  (*Write64)(VAddr va, u64 value);
    };

    using HostCallFunc = void* (HostCallbacks::*);

}
