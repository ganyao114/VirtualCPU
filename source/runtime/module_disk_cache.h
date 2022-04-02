//
// Created by 甘尧 on 2022/1/26.
//

#include "include/types.h"
#include "base/file.h"
#include "include/svm_instance.h"
#include <array>
#include <vector>
#include <unordered_map>
#include <platform/unix_file.h>

namespace Svm {

    constexpr std::array<char, 4> MAGIC{'s', 'v', 'm', 'c'};
    constexpr auto CURRENT_VER = 1;
    constexpr auto X86_64 = 0;
    constexpr auto AARCH64 = 1;

    struct CacheHeader {
        std::array<char, 4> magic;
        u32 version;
        u32 guest_arch;
        u32 host_arch;
        u64 cache_hash;
        VAddr mapped_address;
        size_t mapped_size;
        size_t function_table_offset;
        u32 function_count;
        size_t code_cache_offset;
        size_t code_cache_size;
    };

    struct FunctionEntry {
        size_t module_offset;
        size_t code_cache_offset;
    };

    struct CacheParams {
        VAddr mapped_address;
        size_t mapped_size;
        std::vector<FunctionEntry> entries;
        std::vector<u8> code_cache_memory;
    };

    class ModuleCache {
    public:

        explicit ModuleCache(const std::string& cache_path);

        bool Open();

        bool Load(size_t load_addr, Runtime *runtime);

        bool Save(CacheParams &module);

        size_t CacheSize();

    private:

        bool MapCache(size_t addr);

        UnixFile file;
        CacheHeader header;
    };

}
