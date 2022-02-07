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
#include "base/object_pool.h"

using namespace Svm::X86;

extern "C" __attribute__((visibility("default"))) void abort(void) {
    printf("abort\n");
    exit(0);
}

struct CacheEntry {
    size_t base;
    size_t size;
};

class MyVCpu : public Svm::VCpu {
public:
    MyVCpu(Svm::SvmInstance *instance) : VCpu(instance) {}

    void CallSvc(Svm::u32 num) override {
        printf("CallSvc: %d \n", num);
    }

    void CallBrk(Svm::u32 num) override {
        printf("CallBrk: %d \n", num);
        AdvancePC(1);
        MakeInterFastReturn();
    }

    void Yield() override {
        printf("Yield \n");
    }
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
    masm.int3();
    masm.jmp(&l2);
    masm.Bind(&l1);
    masm.int3();
    masm.subl(Register::R8, Address(Register::R9, 100));
    masm.cmov(Condition::kParityEven, Register::R8, Register::R10);
    masm.jmp(&l1);
    masm.ret();

    masm.FinalizeCode();
    auto pc = (Svm::VAddr)masm.CodeBufferBaseAddress();

    printf("OFFSET_V: %lu \n", OFFSET_OF(ThreadContext64, xmms));
    printf("OFFSET_XF: %lu \n", OFFSET_OF(ThreadContext64, flags));
    printf("OFFSET_PT: %lu \n", OFF_HELP(page_table));
    printf("OFFSET_NZCV: %lu \n", OFF_HELP(cpu_flags));
    printf("OFFSET_CODE_CACHE: %lu \n", OFF_HELP(code_cache));

    auto test_code_page = static_cast<Svm::u8 *>(Svm::Platform::MapCowMemory(0x1000));
    auto test_rw_page = static_cast<Svm::u8 *>(Svm::Platform::MapCowMemory(0x1000));
    std::memcpy(test_code_page, reinterpret_cast<const void *>(pc), masm.GetBuffer()->Size());

    Svm::UserConfigs configs;

    configs.guest_arch = Svm::CpuArch::X64;
    configs.jit_threads = 2;
    configs.use_soft_mmu = true;
    configs.use_offset_pt = false;
    configs.page_fatal = true;
    configs.check_halt = true;
    configs.tick_count = false;
    configs.static_code = true;
    configs.rsb_cache = false;
    configs.page_align_check = false;
    configs.page_bits = 12;
    configs.address_width = 38;

    Svm::SvmInstance instance(configs);

    auto page_table = instance.PageTable();

    page_table[0x800000 >> 12] = {(Svm::PAddr)test_code_page, Svm::PageEntry::Read};
    page_table[rw_addr >> 12] = {(Svm::PAddr)test_rw_page, Svm::PageEntry::Read};

    MyVCpu core1{&instance};
    MyVCpu core2{&instance};
    MyVCpu core3{&instance};
    MyVCpu core4{&instance};

    auto t1 = std::thread([&]() {
        core1.SetPC(0x800000);
        while (true) {
            core1.ClearInterrupt();
            core1.Run();
        }
    });

    auto t2 = std::thread([&]() {
        core2.SetPC(0x800000);
        while (true) {
            core2.ClearInterrupt();
            core2.Run();
        }
    });

    auto t3 = std::thread([&]() {
        core3.SetPC(0x800000);
        while (true) {
            core3.ClearInterrupt();
            core3.Run();
        }
    });

    auto t4 = std::thread([&]() {
        core4.SetPC(0x800000);
        while (true) {
            core4.ClearInterrupt();
            core4.Run();
        }
    });

    while (true) {
        sleep(3);
        core1.Halt();
        core2.Halt();
        core3.Halt();
        core4.Halt();
    }

    return 1;
}
