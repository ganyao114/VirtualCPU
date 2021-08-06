//
// Created by SwiftGan on 2020/8/20.
//

#pragma once

#include "base/marco.h"

namespace Svm::Platform {

    void *MirrorRWMemory(void *base, size_t size);

    void *MapExecutableMemory(size_t size, VAddr addr = 0);

    void *MapCowMemory(size_t size, VAddr addr = 0);

    void *MapFile(int fd, size_t size, size_t offset = 0);

    void UnMapMemory(VAddr addr, size_t size);

    void ClearICache(VAddr start, size_t size);

    void ClearDCache(VAddr start, size_t size);
}
