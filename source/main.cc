//
// Created by SwiftGan on 2020/12/27.
//

#include "main.h"

#include <cstdio>
#include <asm/x86/64/assembler_x86_64.h>
#include <include/svm_cpu.h>
#include <include/svm_instance.h>
#include <backend/cpu.h>
#include <platform/memory.h>
#include <libc.h>
#include <thread>

using namespace Svm::X86;

extern "C" __attribute__((visibility("default"))) void abort(void) {
    printf("abort\n");
    exit(0);
}

struct CacheEntry {
    size_t base;
    size_t size;
};

int main(int argc, char *argv[]) {
    X86_64Assembler masm{};
    auto rw_addr = 0x8000000;

    NearLabel l1, l2;
    masm.movq(Register::R10, Immediate(rw_addr));
    masm.movq(Register::RAX, Immediate(rw_addr));
    masm.Bind(&l2);
    masm.movq(Register::R8, Immediate(rw_addr));
    masm.movl(Register::RAX, Address(Register::R8, 0x10));
    masm.subq(Register::R10, Immediate(1));
    masm.subq(Register::R10, Immediate(1));
    masm.subq(Register::R10, Immediate(1));
    masm.subq(Register::R10, Immediate(1));
    masm.jmp(&l2);
    masm.Bind(&l1);
    masm.int3();
    masm.subl(Register::R8, Address(Register::R9, 100));
    masm.cmov(Condition::kParityEven, Register::R8, Register::R10);
    masm.jmp(&l1);
    masm.int3();
    masm.ret();

    masm.FinalizeCode();
    Svm::VAddr pc = (Svm::VAddr)masm.CodeBufferBaseAddress();

    printf("OFFSET_V: %d \n", OFFSET_OF(ThreadContext64, xmms));
    printf("OFFSET_XF: %d \n", OFFSET_OF(ThreadContext64, flags));
    printf("OFFSET_PT: %d \n", OFF_HELP(page_table));
    printf("OFFSET_NZCV: %d \n", OFF_HELP(cpu_flags));
    printf("OFFSET_CODE_CACHE: %d \n", OFF_HELP(code_cache));


    //auto res = Svm::BuildFunction(pc, nullptr);

    auto test_code_page = static_cast<Svm::u8 *>(Svm::Platform::MapCowMemory(0x1000));
    auto test_rw_page = static_cast<Svm::u8 *>(Svm::Platform::MapCowMemory(0x1000));
    std::memcpy(test_code_page, reinterpret_cast<const void *>(pc), masm.GetBuffer()->Size());

    Svm::UserConfigs configs;

    configs.guest_arch = Svm::CpuArch::X64;
    configs.jit_threads = 2;
    configs.use_offset_pt = false;
    configs.page_fatal = true;
    configs.check_halt = true;
    configs.tick_count = false;
    configs.static_code = false;
    configs.rsb_cache = false;
    configs.page_align_check = false;
    configs.page_bits = 12;
    configs.address_width = 36;

    Svm::SvmInstance instance(configs);

    auto page_table = instance.PageTable();

    page_table[0x800000 >> 12] = {(Svm::PAddr)test_code_page, Svm::PageEntry::Read};
    page_table[rw_addr >> 12] = {(Svm::PAddr)test_rw_page, Svm::PageEntry::Read};

    auto t1 = std::thread([&]() {
        Svm::VCpu current_core{&instance};
        current_core.SetPC(0x800000);
        current_core.Run();
    });

    auto t2 = std::thread([&]() {
        Svm::VCpu current_core{&instance};
        current_core.SetPC(0x800000);
        current_core.Run();
    });

    auto t3 = std::thread([&]() {
        Svm::VCpu current_core{&instance};
        current_core.SetPC(0x800000);
        current_core.Run();
    });

    auto t4 = std::thread([&]() {
        Svm::VCpu current_core{&instance};
        current_core.SetPC(0x800000);
        current_core.Run();
    });

    while (true) {
        sleep(1);
    }

    return 1;
}
