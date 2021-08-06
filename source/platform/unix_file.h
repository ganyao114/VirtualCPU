//
// Created by SwiftGan on 2020/12/16.
//

#pragma once

#include <base/file.h>

namespace Svm {

    class UnixFile : public File {
    public:

        UnixFile(const std::string &path);

        ~UnixFile() override;

        bool Open(Mode mode) override;

        bool Exist() override;

        void Create() override;

        bool Resize(size_t new_size) override;

        bool Close() override;

        void Read(void *dest, size_t offset, size_t size) override;

        void Write(void *src, size_t offset, size_t size) override;

        void *Map(size_t offset, size_t size) override;

    private:
        int fd_{};
        std::string path_;
        std::list<std::pair<void*, size_t>> maps_;
    };

}
