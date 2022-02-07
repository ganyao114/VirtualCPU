//
// Created by 甘尧 on 2022/1/26.
//

#include "module_disk_cache.h"

namespace Svm {

    ModuleCache::ModuleCache(std::string cache_path) : file{std::move(cache_path)} {}

    bool ModuleCache::Open() {
        if (!file.Exist()) {
            return false;
        }
        if (!file.Open(File::ReadWrite)) {
            return false;
        }
        header = file.Read<CacheHeader>();
        if (header.magic != MAGIC) {
            return false;
        }
        if (header.arch != AARCH64) {
            return false;
        }
        if (header.version != CURRENT_VER) {
            return false;
        }
        if (!header.function_count) {
            return false;
        }
        if (!header.code_cache_size) {
            return false;
        }
        return true;
    }

    bool ModuleCache::Load(size_t load_addr, Runtime *runtime) {
        if (!MapCache(load_addr)) {
            return false;
        }
        for (u32 i = 0; i < header.function_count; ++i) {
            auto offset = header.function_table_offset + i * sizeof(FunctionEntry);
            auto entry = file.Read<FunctionEntry>(offset);
            auto cache_address = load_addr + entry.code_cache_offset;
            runtime->PutCodeCache(entry.module_offset + header.mapped_address, cache_address);
        }
        return true;
    }

    bool ModuleCache::Save(CacheParams &module) {
        auto page_size = getpagesize();
        header.mapped_address = module.mapped_address;
        header.mapped_size = module.mapped_size;
        header.code_cache_size = AlignUp(module.code_cache_memory.size(), page_size);
        header.function_count = module.entries.size();
        if (!file.Exist()) {
            file.Create();
        }

        if (!file.Open(File::ReadWrite)) {
            return false;
        }

        header.magic = MAGIC;
        header.arch = AARCH64;
        header.version = CURRENT_VER;
        header.function_table_offset = sizeof(header);
        header.code_cache_offset = header.function_table_offset + header.function_count * sizeof(FunctionEntry);

        auto res = file.Resize(sizeof(CacheHeader) + sizeof(FunctionEntry) * header.function_count + header.code_cache_size);
        if (!res) return false;
        res = file.Write(module.entries.data(), header.function_table_offset, module.entries.size() * sizeof(FunctionEntry));
        if (!res) return false;
        res = file.Write(module.code_cache_memory.data(), header.code_cache_offset, module.code_cache_memory.size());
        if (!res) return false;
        res = file.Write(header, 0);
        if (!res) return false;

        file.Close();

        return true;
    }

    bool ModuleCache::MapCache(size_t addr) {
        return file.Map(header.code_cache_offset, header.code_cache_size, true, addr);
    }

    size_t ModuleCache::CacheSize() {
        return header.code_cache_size;
    }

}