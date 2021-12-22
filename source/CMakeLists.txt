cmake_minimum_required(VERSION 3.4.1)
project(svm)
ENABLE_LANGUAGE(ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(externals)

add_subdirectory(externals/distorm)
target_include_directories(distorm INTERFACE externals/distorm)

add_subdirectory(externals/vixl)
target_include_directories(vixl INTERFACE externals/vixl)

option(BUILD_CMD "Build Command line" ON)

set(SOURCES
        base/marco.cc
        asm/x86/assembler.cc
        asm/x86/memory_region.cc
        asm/x86/64/assembler_x86_64.cc
        asm/x86/64/managed_register_x86_64.cc
        ir/operand.cc
        ir/instruction.cc
        ir/block.cc
        ir/assembler.cc
        ir/interpreter.cc
        ast/ast.cc
        ast/cfg.cc
        ast/expr.cc
        ast/block.cc
        ast/graph.cc
        frontend/x64/decoder.cc
        frontend/x64/jit_visitor.cc
        frontend/function_disam.cc
        frontend/arm32/decoder.cc
        ir_opt/ir_opt_result.cc
        ir_opt/dead_code_remove.cc
        ir_opt/ctx_get_set_elimination.cc
        ir_opt/value_expr_fold.cc
        ir_opt/const_memory_read_pass.cc
        ir_opt/const_folding_pass.cc
        ir_opt/register_alloc_pass.cc
        ir_opt/flags_get_set_sync.cc
        ir_opt/flags_set_elimination.cc
        platform/memory.cc
        platform/unix_file.cc
        runtime/jit_runtime.cc
        runtime/basic_block.cc
        runtime/jit_thread.cc
        runtime/instance.cc
        runtime/vcpu.cc
        cache/cache_module.cc
        cache/cache_pool.cc

        backend/arm64/ir_translator.cc
        backend/register_manager.cc
        backend/label_manager.cc
        ir_opt/ir_opt_manager.cc
        backend/arm64/jit_context.cc
        backend/arm64/label_manager.cc
        backend/arm64/register_manager.cc
        backend/arm64/trampolines.cc
        backend/arm64/trampolines.S
        backend/arm64/svm_context.cc
        backend/arm64/ir_translate_status.cc
        backend/arm64/ir_translate_memory.cc
        backend/arm64/ir_translate_simd.cc
        backend/arm64/ir_translate_logical.cc
        backend/arm64/ir_translate_system.cc
        backend/arm64/ir_translate_context.cc
        backend/arm64/x64/ir_translate_x64.cc)

if (ANDROID)
    set(PLATFORM_SOURCES android_entry.cc)
elseif (BUILD_CMD)
    set(PLATFORM_SOURCES main.cc)
else()
endif ()

if (ANDROID)
    add_library(svm SHARED
            ${SOURCES}
            ${PLATFORM_SOURCES})
elseif (BUILD_CMD)
    add_executable(svm
            ${SOURCES}
            ${PLATFORM_SOURCES})

else()
endif ()

target_link_libraries(svm PRIVATE distorm)
target_link_libraries(svm PRIVATE vixl)

if (ANDROID)
    target_link_libraries(svm android log)
endif ()
