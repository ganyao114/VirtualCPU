//
// Created by SwiftGan on 2020/12/16.
//
#include <fcntl.h>
#include <unistd.h>
#include "unix_file.h"
#include "memory.h"
#include <sys/stat.h>

namespace Svm {

    UnixFile::UnixFile(const std::string &path) : path_(path) {}

    UnixFile::~UnixFile() {
        Close();
    }

    bool UnixFile::Open(Mode mode) {
        assert(path_ != "");
        int unix_mode{};

        if (mode == Mode::ReadWrite) {
            unix_mode |= O_RDWR;
        } else if (mode == Mode::Readable) {
            unix_mode |= O_RDONLY;
        } else if (mode == Mode::Writeable) {
            unix_mode |= O_WRONLY;
        }

        if (mode & Mode::Append) {
            unix_mode |= O_APPEND;
        }

        fd_ = open(path_.c_str(), unix_mode);

        return fd_ > 0;
    }

    bool UnixFile::Close() {
        if (fd_ > 0) {
            for (auto [start, size] : maps_) {
                Platform::UnMapMemory(reinterpret_cast<VAddr>(start), size);
            }
            maps_.clear();
            auto res = close(fd_) != -1;
            if (res) {
                fd_ = -1;
            }
            return res;
        } else {
            return false;
        }
    }

    bool UnixFile::Read(void *dest, size_t offset, size_t size) {
        return pread(fd_, dest, size, offset) > 0;
    }

    bool UnixFile::Write(void *src, size_t offset, size_t size) {
        return pwrite(fd_, src, size, offset) > 0;
    }

    bool UnixFile::Resize(size_t new_size) {
        if (!fd_) {
            return false;
        }
        auto res = ftruncate(fd_, new_size) == 0;
        return res;
    }

    void *UnixFile::Map(size_t offset, size_t size) {
        auto addr = Platform::MapFile(fd_, size, offset);
        if (addr) {
            maps_.emplace_back(addr, size);
        }
        return addr;
    }

    bool UnixFile::Exist() {
        struct stat file_info;
        int result = stat(path_.c_str(), &file_info);
        return result == 0;
    }

    void UnixFile::Create() {
        int fd = open(path_.c_str(), O_CREAT|O_RDWR, 0777);
        if (fd) {
            close(fd);
        }
    }

}
