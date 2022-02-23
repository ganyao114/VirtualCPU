//
// Created by swift on 2021/6/30.
//

#pragma once

#include "types.h"

namespace Svm::Memory {

    constexpr auto page_entry_attr_bits = 3;
    constexpr auto readable_bit = 0;
    constexpr auto writable_bit = 1;
    constexpr auto executable_bit = 2;

    union PageEntry {

        enum Attr : u8 {
            None    = 0,
            Read    = 1 << readable_bit,
            Write   = 1 << writable_bit,
            Exe     = 1 << executable_bit,
        };

        PAddr pte{};
        struct {
            PAddr attrs: page_entry_attr_bits;
            PAddr index: sizeof(PAddr) * 8 - page_entry_attr_bits;
        };

        constexpr PageEntry(PAddr pte) : pte{pte} {}

        constexpr PageEntry(PAddr page_start, u8 attr) : index{page_start >> page_entry_attr_bits}, attrs{attr} {}

        constexpr PAddr PageStart() {
            return index << page_entry_attr_bits;
        }

        constexpr bool Readable() {
            return attrs & Read;
        }

        constexpr bool Writeable() {
            return attrs & Write;
        }

        constexpr bool Executable() {
            return attrs & Exe;
        }
    };

}
