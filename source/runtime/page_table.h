//
// Created by SwiftGan on 2020/9/14.
//

#pragma once

#include <base/marco.h>
#include <optional>
#include <platform/memory.h>
#include <base/cow_vector.h>
#include "include/memory_interface.h"
#include "include/page_entry.h"

namespace Svm::Memory {

    class PageTableConst {
    public:

        explicit PageTableConst(const u8 addr_width, const u8 page_bits) : addr_width(
                addr_width), page_bits(page_bits), page_size(1 << page_bits), page_mask(page_size - 1) {}

        const u8 addr_width;
        const u8 page_bits;
        const u32 page_size;
        const VAddr page_mask;
    };

    class BasePageTable : public PageTableConst, public MemoryInterface, CopyDisable {
    public:

        explicit BasePageTable(const u8 addr_witdh, const u8 page_bits) : PageTableConst(addr_witdh, page_bits) {
            page_table_entries = 1ULL << (addr_width - page_bits);
        }

        virtual void Initialize() = 0;

        virtual void Destroy() = 0;

        virtual PageEntry *PageTablePtr() = 0;

        virtual PageEntry &GetPTE(size_t page_index) = 0;

        virtual void MapPage(size_t page_index, const PageEntry &pte) = 0;

        virtual void UnmapPages(size_t page_index, size_t num_pages = 1) = 0;

        [[nodiscard]] constexpr u32 PageEntries() const {
            return page_table_entries;
        }

    protected:

        std::size_t page_table_entries;
    };

    class FlatPageTable : public BasePageTable {
    public:

        explicit FlatPageTable(const u8 addr_width, const u8 page_bits) : BasePageTable(addr_width, page_bits) {}

        void Initialize() override {
            pages.Resize(this->page_table_entries);
            assert(pages.DataRW());
        }

        void Destroy() override {};

        PageEntry *PageTablePtr() override {
            return pages.DataRW();
        }

        void ReadMemory(const VAddress &src_addr, void *dest_buffer, size_t size) override {
            if (std::holds_alternative<u64>(src_addr)) {
                ReadImpl(std::get<u64>(src_addr), dest_buffer, size);
            } else {
                ReadImpl(std::get<u32>(src_addr), dest_buffer, size);
            }
        }

        void WriteMemory(const VAddress &dest_addr, const void* src_buffer, size_t size) override {
            if (std::holds_alternative<u64>(dest_addr)) {
                WriteImpl(std::get<u64>(dest_addr), src_buffer, size);
            } else {
                WriteImpl(std::get<u32>(dest_addr), src_buffer, size);
            }
        }

        std::optional<void *> GetPointer(const VAddress &vaddr) override {
            if (std::holds_alternative<u64>(vaddr)) {
                return GetPointerImpl(std::get<u64>(vaddr));
            } else {
                return GetPointerImpl(std::get<u32>(vaddr));
            }
        }

        template<typename Addr>
        void ReadImpl(const Addr src_addr, void *dest_buffer, size_t size) {
            Addr remaining_size = size;
            Addr page_index = src_addr >> this->page_bits;
            Addr page_offset = src_addr & this->page_mask;

            while (remaining_size > 0) {
                const std::size_t copy_amount =
                        std::min(static_cast<Addr>(this->page_size) - page_offset,
                                 remaining_size);

                auto &pte = GetPTE(page_index);

                if (pte.Readable()) {
                    VAddr src_ptr = pte.PageStart() + page_offset;
                    std::memcpy(dest_buffer, reinterpret_cast<const void *>(src_ptr),
                                copy_amount);
                } else {
                    throw MemoryException(MemoryException::Read, (page_index << this->page_bits) + page_offset);
                }

                page_index++;
                page_offset = 0;
                dest_buffer = static_cast<u8 *>(dest_buffer) + copy_amount;
                remaining_size -= copy_amount;
            }
        }

        template<typename Addr>
        void WriteImpl(Addr dest_addr, const void* src_buffer, size_t size) {
            Addr remaining_size = size;
            Addr page_index = dest_addr >> this->page_bits;
            Addr page_offset = dest_addr & this->page_mask;

            while (remaining_size > 0) {
                const std::size_t copy_amount =
                        std::min(static_cast<Addr>(this->page_size) - page_offset,
                                 remaining_size);

                auto &pte = GetPTE(page_index);

                if (pte.Writeable()) {
                    VAddr dest_ptr = pte.PageStart() + page_offset;
                    std::memcpy(reinterpret_cast<void *>(dest_ptr), src_buffer, copy_amount);
                } else {
                    throw MemoryException(MemoryException::Write, (page_index << this->page_bits) + page_offset);
                }

                page_index++;
                page_offset = 0;
                src_buffer = static_cast<const u8*>(src_buffer) + copy_amount;
                remaining_size -= copy_amount;
            }
        }

        virtual PageEntry &GetPTE(size_t page_index) override {
            return pages[page_index];
        }

        template<typename Addr>
        std::optional<void *> GetPointerImpl(Addr vaddr) {
            auto &pte = GetPTE(vaddr >> this->page_bits);
            auto page_start = pte.PageStart();
            if (!page_start) {
                return {};
            }
            Addr page_offset = vaddr & this->page_mask;
            return std::optional<void *>(reinterpret_cast<void*>(page_start + page_offset));
        }

        void MapPage(size_t page_index, const PageEntry &pte) override {
            pages[page_index] = pte;
        }

        void UnmapPages(size_t page_index, size_t num_pages) override {
            while (num_pages > 0) {
                pages[page_index] = 0;
                num_pages--;
                page_index++;
            }
        }

    private:
        CowVector<PageEntry> pages;
    };

}
