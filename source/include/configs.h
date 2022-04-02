//
// Created by swift on 2021/5/16.
//

#pragma once

namespace Svm {

    struct UserConfigs {
        CpuArch guest_arch{CpuArch::Ukn};
        bool use_offset_pt{};
        bool use_soft_mmu{};
        bool page_fatal{};
        bool page_align_check{};
        bool check_halt{};
        bool tick_count{};
        bool static_code{};
        bool disk_code_cache{};
        bool rsb_cache{};
        bool accurate_memory{};
        bool inline_dispatcher{};
        u8 page_bits{};
        u8 address_width{};
        u8 jit_threads{3};
        void *offset_pt_base{};
    };

}
