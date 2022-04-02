//
// Created by SwiftGan on 2020/12/16.
//
#pragma once

#include "marco.h"

namespace Svm {

    class File {
    public:

        enum Mode {
            None        = 0,
            Mask        = 1,
            Readable    = 1 << 0,
            Writeable   = 1 << 1,
            ReadWrite   = Readable | Writeable,
            Append      = 1 << 2,
            WriteAppend = Readable | Append
        };

        virtual ~File() = default;

        virtual bool Open(Mode mode) = 0;

        virtual bool Resize(size_t new_size) = 0;

        virtual bool Exist() = 0;

        virtual void Create() = 0;

        virtual bool Close() = 0;

        virtual bool Read(void *dest, size_t offset, size_t size) = 0;

        virtual bool Write(void *src, size_t offset, size_t size) = 0;

        virtual void* Map(size_t offset, size_t size) = 0;

        template<typename T>
        T Read(size_t offset = 0) {
            T t;
            Read(&t, offset, sizeof(T));
            return std::move(t);
        }

        template<typename T>
        void Write(T &t, size_t offset = 0) {
            Write(&t, offset, sizeof(T));
        }
    };

}
