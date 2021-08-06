//
// Created by swift on 2021/6/30.
//
#include <cstdio>
#include <asm/x86/64/assembler_x86_64.h>
#include <backend/label_manager.h>
#include <ir_opt/ir_opt_manager.h>
#include <frontend/function_disam.h>
#include <include/svm_cpu.h>
#include <include/svm_instance.h>
#include <jni.h>

using namespace Svm::Decoder;
using namespace Svm::X86;
using namespace Svm::Cache;

struct CacheEntry {
    size_t base;
    size_t size;
};

void Test(JNIEnv *env, jclass clazz) {
    X86_64Assembler masm{};

    NearLabel l1;
    masm.movq(Register::R8, Address(Register::R9, ScaleFactor::TIMES_4, 100));
    masm.addw(Address(Register::R9, 100), Immediate(100));
    masm.Bind(&l1);
    masm.subl(Register::R8, Address(Register::R9, 100));
    masm.cmov(Condition::kParityEven, Register::R8, Register::R10);
    masm.j(Condition::kLessEqual, &l1);
    masm.subl(Register::R8, Address(Register::R9, 100));
    masm.subl(Register::R8, Address(Register::R9, 100));
    masm.call(Address(Register::RIP, 100));
    masm.subl(Register::R8, Address(Register::R9, 100));
    masm.subl(Register::R8, Address(Register::R9, 100));
    masm.ret();

    masm.FinalizeCode();
    Svm::VAddr pc = (Svm::VAddr)masm.CodeBufferBaseAddress();

    printf("OFFSET_V: %d \n", OFFSET_OF(ThreadContext64, xmms));
    printf("OFFSET_XF: %d \n", OFFSET_OF(ThreadContext64, flags));
    printf("OFFSET_PT: %d \n", OFF_HELP(page_table));
    printf("OFFSET_NZCV: %d \n", OFF_HELP(cpu_flags));
    printf("OFFSET_CODE_CACHE: %d \n", OFF_HELP(code_cache));


    auto res = Svm::BuildFunction(pc, nullptr);

    Svm::u8 *test_code_page = static_cast<Svm::u8 *>(Svm::Platform::MapCowMemory(0x1000));
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

    Svm::VCpu current_core{&instance};
    current_core.SetPC(0x800000);


    current_core.Run();

}

static bool registerNativeMethods(JNIEnv *env, const char *className, JNINativeMethod *jniMethods, int methods) {
    jclass clazz = env->FindClass(className);
    if (clazz == nullptr) {
        return false;
    }
    return env->RegisterNatives(clazz, jniMethods, methods) >= 0;
}

static JNINativeMethod JNISvmMethods[] = {
        {"test",                   "()V", (void *) Test}
};

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {

    constexpr auto  CLASS_SVM = "com/swift/openrosseta/MainActivity";

    JNIEnv *env = nullptr;

    if (vm->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
        return -1;
    }

    if (!registerNativeMethods(env, CLASS_SVM, JNISvmMethods, sizeof(JNISvmMethods) / sizeof(JNINativeMethod))) {
        return -1;
    }

    return JNI_VERSION_1_6;
}


