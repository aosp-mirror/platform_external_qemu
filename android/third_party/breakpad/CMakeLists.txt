cmake_minimum_required(VERSION 3.5)
project(breakpad)

if(NOT ANDROID_QEMU2_TOP_DIR)
  get_filename_component(ANDROID_QEMU2_TOP_DIR
                         "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)
  get_filename_component(
    ADD_PATH "${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/" ABSOLUTE)
  list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
endif()
include(android)
prebuilt(THREADS)

if(NOT TARGET gmock_main)
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/android/third_party/googletest
                   googletest)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Configure the compiler for breakpad:
if(DARWIN_X86_64)
  add_c_flag("-fexceptions")
  add_c_flag("-frtti")
  add_c_flag("-fmessage-length=0")
  add_c_flag("-fdiagnostics-show-note-include-stack")
  add_c_flag("-fmacro-backtrace-limit=0")
  add_c_flag("-fobjc-weak")
  add_c_flag("-fpascal-strings")
  add_c_flag("-fno-common")
  add_c_flag("-Wno-unused-command-line-argument")
  # Optimizations will cause the compiler to optimize away some crucial breakpad
  # behavior, so let's not do that.
  add_c_flag("-O0")
elseif(LINUX)
  add_c_flag("-fexceptions")
  add_c_flag("-frtti")
elseif(WINDOWS_MSVC_X86_64)
  add_c_flag("-Wno-c++17-narrowing")
else()
  message(
    FATAL_ERROR
      "Only darwin, linux or windows using clang msvc are supported, not ${ANDROID_TARGET_TAG}."
  )
endif()

set(ANDROID_BREAKPAD_WINDOWS_DEFS
    -D_WIN32_WINNT=0x0600
    -DWINVER=0x0600
    -DWIN32
    -D_WINDOWS
    -D_HAS_EXCEPTIONS=0
    -DNOMINMAX
    -D_CRT_RAND_S
    -DCERT_CHAIN_PARA_HAS_EXTRA_FIELDS
    -DWIN32_LEAN_AND_MEAN
    -D_SECURE_ATL
    -D_CRT_SECURE_NO_DEPRECATE
    -D_CRT_NONSTDC_NO_WARNINGS
    -D_CRT_NONSTDC_NO_DEPRECATE
    -D_USING_V170_SDK71_
    -DNDEBUG
    -DNVALGRIND
    -DOFFICIAL_BUILD
    -D_UNICODE
    -DUNICODE)
# Make sure we do not have definitions that will break us.
remove_definitions(
  "-DNDEBUG"
  "-DNDEBUG=1"
  "-D_FILE_OFFSET_BITS=64"
  "-D_GNU_SOURCE=1"
  "-D_LARGEFILE_SOURCE"
  "-D_LIBCPP_VERSION=__GLIBCPP__"
  "-D__STDC_CONSTANT_MACROS"
  "-D__STDC_FORMAT_MACROS=1"
  "-D__STDC_LIMIT_MACROS=1"
  "-DANDROID_DEBUG")
# Definitely make sure this is off, unless you want bizarre failures
# everywhere..
add_definitions("-UNDEBUG")
add_definitions("-UANDROID_DEBUG")

# Unit tests please!
enable_testing()

# Location of the root of the breakpad source.
set(BREAKPAD_SRC # cmake-format: sortable
                 ${ANDROID_QEMU2_TOP_DIR}/../google-breakpad/src)

android_add_library(
  TARGET breakpad_disasm
  LIBNAME
    breakpad
    URL
    "https://android.googlesource.com/platform/external/google-breakpad/+/refs/heads/emu-master-dev"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../google-breakpad"
  NOTICE "REPO/LICENSE"
  LICENSE "BSD-3-Clause"
  SRC # cmake-format: sortable
      ${BREAKPAD_SRC}/third_party/libdisasm/ia32_implicit.c
      ${BREAKPAD_SRC}/third_party/libdisasm/ia32_insn.c
      ${BREAKPAD_SRC}/third_party/libdisasm/ia32_invariant.c
      ${BREAKPAD_SRC}/third_party/libdisasm/ia32_modrm.c
      ${BREAKPAD_SRC}/third_party/libdisasm/ia32_opcode_tables.c
      ${BREAKPAD_SRC}/third_party/libdisasm/ia32_operand.c
      ${BREAKPAD_SRC}/third_party/libdisasm/ia32_reg.c
      ${BREAKPAD_SRC}/third_party/libdisasm/ia32_settings.c
      ${BREAKPAD_SRC}/third_party/libdisasm/x86_disasm.c
      ${BREAKPAD_SRC}/third_party/libdisasm/x86_format.c
      ${BREAKPAD_SRC}/third_party/libdisasm/x86_imm.c
      ${BREAKPAD_SRC}/third_party/libdisasm/x86_insn.c
      ${BREAKPAD_SRC}/third_party/libdisasm/x86_misc.c
      ${BREAKPAD_SRC}/third_party/libdisasm/x86_operand_list.c)
set_property(TARGET breakpad_disasm PROPERTY CXX_STANDARD 17)
android_target_compile_definitions(breakpad_disasm windows PRIVATE
                                   ${ANDROID_BREAKPAD_WINDOWS_DEFS})

android_target_compile_options(breakpad_disasm windows
                               PRIVATE -Wno-int-to-void-pointer-cast)
#
# breakpad_common

android_add_library(
  TARGET breakpad_common
  LIBNAME
    breakpad
    URL
    "https://android.googlesource.com/platform/external/google-breakpad/+/refs/heads/emu-master-dev"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../google-breakpad"
  NOTICE "REPO/LICENSE"
  LICENSE "BSD-3-Clause"
  SRC # cmake-format: sortable
      ${BREAKPAD_SRC}/common/convert_UTF.cc
      ${BREAKPAD_SRC}/common/long_string_dictionary.cc
      ${BREAKPAD_SRC}/common/md5.cc
      ${BREAKPAD_SRC}/common/module.cc
      ${BREAKPAD_SRC}/common/simple_string_dictionary.cc
      ${BREAKPAD_SRC}/common/string_conversion.cc
      ${BREAKPAD_SRC}/common/test_assembler.cc
  WINDOWS ${BREAKPAD_SRC}/common/windows/dia_util.cc
          ${BREAKPAD_SRC}/common/windows/guid_string.cc
          ${BREAKPAD_SRC}/common/windows/http_upload.cc
          ${BREAKPAD_SRC}/common/windows/omap.cc
          ${BREAKPAD_SRC}/common/windows/pdb_source_line_writer.cc
          ${BREAKPAD_SRC}/common/windows/pe_source_line_writer.cc
          ${BREAKPAD_SRC}/common/windows/pe_util.cc
          ${BREAKPAD_SRC}/common/windows/string_utils.cc
          ${BREAKPAD_SRC}/common/windows/symbol_collector_client.cc
  LINUX ${BREAKPAD_SRC}/common/dwarf/bytereader.cc
        ${BREAKPAD_SRC}/common/dwarf/cfi_assembler.cc
        ${BREAKPAD_SRC}/common/dwarf/dwarf2diehandler.cc
        ${BREAKPAD_SRC}/common/dwarf/dwarf2reader.cc
        ${BREAKPAD_SRC}/common/dwarf/elf_reader.cc
        ${BREAKPAD_SRC}/common/dwarf/functioninfo.cc
        ${BREAKPAD_SRC}/common/dwarf_cfi_to_module.cc
        ${BREAKPAD_SRC}/common/dwarf_cu_to_module.cc
        ${BREAKPAD_SRC}/common/dwarf_line_to_module.cc
        ${BREAKPAD_SRC}/common/dwarf_range_list_handler.cc
        ${BREAKPAD_SRC}/common/language.cc
        ${BREAKPAD_SRC}/common/linux/crc32.cc
        ${BREAKPAD_SRC}/common/linux/dump_symbols.cc
        ${BREAKPAD_SRC}/common/linux/elf_core_dump.cc
        ${BREAKPAD_SRC}/common/linux/elf_symbols_to_module.cc
        ${BREAKPAD_SRC}/common/linux/elfutils.cc
        ${BREAKPAD_SRC}/common/linux/file_id.cc
        ${BREAKPAD_SRC}/common/linux/google_crashdump_uploader.cc
        ${BREAKPAD_SRC}/common/linux/guid_creator.cc
        ${BREAKPAD_SRC}/common/linux/http_upload.cc
        ${BREAKPAD_SRC}/common/linux/libcurl_wrapper.cc
        ${BREAKPAD_SRC}/common/linux/linux_libc_support.cc
        ${BREAKPAD_SRC}/common/linux/memory_mapped_file.cc
        ${BREAKPAD_SRC}/common/linux/safe_readlink.cc
        ${BREAKPAD_SRC}/common/linux/synth_elf.cc
        ${BREAKPAD_SRC}/common/path_helper.cc
        ${BREAKPAD_SRC}/common/stabs_reader.cc
        ${BREAKPAD_SRC}/common/stabs_to_module.cc
  DARWIN ${BREAKPAD_SRC}/common/dwarf/bytereader.cc
         ${BREAKPAD_SRC}/common/dwarf/cfi_assembler.cc
         ${BREAKPAD_SRC}/common/dwarf/dwarf2diehandler.cc
         ${BREAKPAD_SRC}/common/dwarf/dwarf2reader.cc
         ${BREAKPAD_SRC}/common/dwarf/elf_reader.cc
         ${BREAKPAD_SRC}/common/dwarf/functioninfo.cc
         ${BREAKPAD_SRC}/common/dwarf_cfi_to_module.cc
         ${BREAKPAD_SRC}/common/dwarf_cu_to_module.cc
         ${BREAKPAD_SRC}/common/dwarf_line_to_module.cc
         ${BREAKPAD_SRC}/common/dwarf_range_list_handler.cc
         ${BREAKPAD_SRC}/common/language.cc
         ${BREAKPAD_SRC}/common/mac/arch_utilities.cc
         ${BREAKPAD_SRC}/common/mac/bootstrap_compat.cc
         ${BREAKPAD_SRC}/common/mac/dump_syms.cc
         ${BREAKPAD_SRC}/common/mac/file_id.cc
         ${BREAKPAD_SRC}/common/mac/GTMLogger.m
         ${BREAKPAD_SRC}/common/mac/HTTPMultipartUpload.m
         ${BREAKPAD_SRC}/common/mac/MachIPC.mm
         ${BREAKPAD_SRC}/common/mac/macho_id.cc
         ${BREAKPAD_SRC}/common/mac/macho_reader.cc
         ${BREAKPAD_SRC}/common/mac/macho_utilities.cc
         ${BREAKPAD_SRC}/common/mac/macho_walker.cc
         ${BREAKPAD_SRC}/common/mac/string_utilities.cc
         ${BREAKPAD_SRC}/common/path_helper.cc
         ${BREAKPAD_SRC}/common/stabs_reader.cc
         ${BREAKPAD_SRC}/common/stabs_to_module.cc)
set_property(TARGET breakpad_common PROPERTY CXX_STANDARD 17)
target_include_directories(breakpad_common PUBLIC ${BREAKPAD_SRC})
android_target_include_directories(breakpad_common linux-x86_64 PUBLIC
                                   ${ANDROID_QEMU2_TOP_DIR}/android)
android_target_include_directories(breakpad_common linux-aarch64 PUBLIC
                                   ${ANDROID_QEMU2_TOP_DIR}/android)
android_target_compile_definitions(breakpad_common linux-x86_64 PUBLIC
                                   -DHAVE_A_OUT_H)
android_target_compile_definitions(breakpad_common linux-aarch64 PUBLIC
                                   -DHAVE_A_OUT_H)
android_target_compile_definitions(breakpad_common darwin-x86_64 PUBLIC
                                   -DHAVE_MACH_O_NLIST_H)
android_target_compile_definitions(breakpad_common windows PRIVATE
                                   ${ANDROID_BREAKPAD_WINDOWS_DEFS})
android_target_link_libraries(breakpad_common linux-x86_64 PRIVATE dl)
android_target_link_libraries(breakpad_common linux-aarch64 PRIVATE dl)
android_target_link_libraries(breakpad_common windows_msvc-x86_64
                              PRIVATE atls::atls)
android_target_link_libraries(
  breakpad_common windows PRIVATE wininet::wininet diaguids::diaguids
                                  dbghelp::dbghelp imagehlp::imagehlp)
# ASAN does not play nice with breakpad tests
if(NOT OPTION_ASAN)
  android_add_test(
    TARGET breakpad_common_unittests
    SRC # cmake-format: sortable
        ${BREAKPAD_SRC}/common/byte_cursor_unittest.cc
        ${BREAKPAD_SRC}/common/long_string_dictionary_unittest.cc
        ${BREAKPAD_SRC}/common/memory_range_unittest.cc
        ${BREAKPAD_SRC}/common/module_unittest.cc
        ${BREAKPAD_SRC}/common/simple_string_dictionary_unittest.cc
        ${BREAKPAD_SRC}/common/test_assembler_unittest.cc
    WINDOWS ${BREAKPAD_SRC}/common/windows/omap_unittest.cc
    DARWIN ${BREAKPAD_SRC}/common/dwarf/bytereader_unittest.cc
           ${BREAKPAD_SRC}/common/dwarf/dwarf2reader_die_unittest.cc
           ${BREAKPAD_SRC}/common/dwarf_cu_to_module_unittest.cc
           ${BREAKPAD_SRC}/common/dwarf_line_to_module_unittest.cc
           ${BREAKPAD_SRC}/common/memory_allocator_unittest.cc
           # ${BREAKPAD_SRC}/common/mac/macho_reader_unittest.cc # Some gmock
           # issues, these fail in xcode as well
           # ${BREAKPAD_SRC}/common/dwarf/dwarf2reader_cfi_unittest.cc
           ${BREAKPAD_SRC}/common/stabs_to_module_unittest.cc
    LINUX ${BREAKPAD_SRC}/common/dwarf/bytereader_unittest.cc
          ${BREAKPAD_SRC}/common/dwarf/dwarf2diehandler_unittest.cc
          ${BREAKPAD_SRC}/common/dwarf/dwarf2reader_cfi_unittest.cc
          ${BREAKPAD_SRC}/common/dwarf/dwarf2reader_die_unittest.cc
          ${BREAKPAD_SRC}/common/dwarf_cfi_to_module_unittest.cc
          ${BREAKPAD_SRC}/common/dwarf_cu_to_module_unittest.cc
          ${BREAKPAD_SRC}/common/dwarf_line_to_module_unittest.cc
          ${BREAKPAD_SRC}/common/linux/dump_symbols_unittest.cc
          ${BREAKPAD_SRC}/common/linux/elf_symbols_to_module_unittest.cc
          ${BREAKPAD_SRC}/common/linux/file_id_unittest.cc
          ${BREAKPAD_SRC}/common/linux/google_crashdump_uploader_test.cc
          ${BREAKPAD_SRC}/common/linux/linux_libc_support_unittest.cc
          ${BREAKPAD_SRC}/common/linux/memory_mapped_file_unittest.cc
          ${BREAKPAD_SRC}/common/linux/safe_readlink_unittest.cc
          ${BREAKPAD_SRC}/common/linux/synth_elf_unittest.cc
          ${BREAKPAD_SRC}/common/linux/tests/crash_generator.cc
          ${BREAKPAD_SRC}/common/memory_allocator_unittest.cc
          ${BREAKPAD_SRC}/common/stabs_reader_unittest.cc
          ${BREAKPAD_SRC}/common/stabs_to_module_unittest.cc
          ${BREAKPAD_SRC}/common/tests/file_utils.cc)
  target_link_libraries(breakpad_common_unittests PRIVATE breakpad_common
                                                          gmock_main)
  android_target_link_libraries(breakpad_common_unittests windows
                                PRIVATE diaguids::diaguids)
  android_target_compile_definitions(breakpad_common_unittests windows PRIVATE
                                     -DUNICODE -D_UNICODE)
  android_target_compile_definitions(breakpad_common_unittests linux-x86_64
                                     PUBLIC -DHAVE_A_OUT_H)
  android_target_compile_definitions(breakpad_common_unittests linux-aarch64
                                     PUBLIC -DHAVE_A_OUT_H)
  android_target_compile_definitions(breakpad_common_unittests darwin-x86_64
                                     PUBLIC -DHAVE_MACH_O_NLIST_H)
endif()
#
# breakpad_processor
android_add_library(
  TARGET breakpad_processor
  LIBNAME
    breakpad
    URL
    "https://android.googlesource.com/platform/external/google-breakpad/+/refs/heads/emu-master-dev"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../google-breakpad"
  NOTICE "REPO/LICENSE"
  LICENSE "BSD-3-Clause"
  SRC # cmake-format: sortable
      ${BREAKPAD_SRC}/processor/basic_code_modules.cc
      ${BREAKPAD_SRC}/processor/basic_source_line_resolver.cc
      ${BREAKPAD_SRC}/processor/call_stack.cc
      ${BREAKPAD_SRC}/processor/cfi_frame_info.cc
      ${BREAKPAD_SRC}/processor/convert_old_arm64_context.cc
      ${BREAKPAD_SRC}/processor/disassembler_x86.cc
      ${BREAKPAD_SRC}/processor/dump_context.cc
      ${BREAKPAD_SRC}/processor/dump_object.cc
      ${BREAKPAD_SRC}/processor/exploitability.cc
      ${BREAKPAD_SRC}/processor/exploitability_linux.cc
      ${BREAKPAD_SRC}/processor/exploitability_win.cc
      ${BREAKPAD_SRC}/processor/fast_source_line_resolver.cc
      ${BREAKPAD_SRC}/processor/logging.cc
      ${BREAKPAD_SRC}/processor/microdump.cc
      ${BREAKPAD_SRC}/processor/microdump_processor.cc
      ${BREAKPAD_SRC}/processor/minidump.cc
      ${BREAKPAD_SRC}/processor/minidump_processor.cc
      ${BREAKPAD_SRC}/processor/module_comparer.cc
      ${BREAKPAD_SRC}/processor/module_serializer.cc
      ${BREAKPAD_SRC}/processor/pathname_stripper.cc
      ${BREAKPAD_SRC}/processor/proc_maps_linux.cc
      ${BREAKPAD_SRC}/processor/process_state.cc
      ${BREAKPAD_SRC}/processor/simple_symbol_supplier.cc
      ${BREAKPAD_SRC}/processor/source_line_resolver_base.cc
      ${BREAKPAD_SRC}/processor/stack_frame_cpu.cc
      ${BREAKPAD_SRC}/processor/stack_frame_symbolizer.cc
      ${BREAKPAD_SRC}/processor/stackwalk_common.cc
      ${BREAKPAD_SRC}/processor/stackwalker.cc
      ${BREAKPAD_SRC}/processor/stackwalker_address_list.cc
      ${BREAKPAD_SRC}/processor/stackwalker_amd64.cc
      ${BREAKPAD_SRC}/processor/stackwalker_arm.cc
      ${BREAKPAD_SRC}/processor/stackwalker_arm64.cc
      ${BREAKPAD_SRC}/processor/stackwalker_mips.cc
      ${BREAKPAD_SRC}/processor/stackwalker_ppc.cc
      ${BREAKPAD_SRC}/processor/stackwalker_ppc64.cc
      ${BREAKPAD_SRC}/processor/stackwalker_sparc.cc
      ${BREAKPAD_SRC}/processor/stackwalker_x86.cc
      ${BREAKPAD_SRC}/processor/symbolic_constants_win.cc
      ${BREAKPAD_SRC}/processor/synth_minidump.cc
      ${BREAKPAD_SRC}/processor/tokenize.cc
  LINUX ${BREAKPAD_SRC}/processor/minidump.cc)
set_property(TARGET breakpad_processor PROPERTY CXX_STANDARD 17)
target_include_directories(breakpad_processor PUBLIC ${BREAKPAD_SRC})
target_link_libraries(breakpad_processor PRIVATE breakpad_disasm
                                                 breakpad_common)
android_target_compile_definitions(breakpad_processor linux-x86_64 PUBLIC
                                   -DHAVE_A_OUT_H)
android_target_compile_definitions(breakpad_processor linux-aarch64 PUBLIC
                                   -DHAVE_A_OUT_H)
android_target_compile_definitions(breakpad_processor darwin-x86_64 PUBLIC
                                   -DHAVE_MACH_O_NLIST_H)

function(add_proc_test name)
  if(CROSSCOMPILE)
    return()
  endif()
  android_add_test(TARGET ${name} SRC # cmake-format: sortable
                                      ${BREAKPAD_SRC}/processor/${name}.cc)
  target_link_libraries(${name} PRIVATE breakpad_processor gmock_main)
  android_target_compile_definitions(${name} windows PRIVATE -DUNICODE
                                     -D_UNICODE)
  set_property(TEST ${name} PROPERTY WORKING_DIRECTORY "${BREAKPAD_SRC}/..")
endfunction()

set(breakpad_processor_unittests
    basic_source_line_resolver_unittest
    cfi_frame_info_unittest
    # disassembler_x86_unittest fast_source_line_resolver_unittest # We
    # explicitly disable rtti in release builds, so we cannot run this one.
    map_serializers_unittest
    microdump_processor_unittest
    minidump_processor_unittest
    minidump_unittest
    range_map_truncate_upper_unittest
    # absl update broke test build stackwalker_address_list_unittest
    # stackwalker_amd64_unittest stackwalker_arm64_unittest
    # stackwalker_arm_unittest stackwalker_x86_unittest
    static_address_map_unittest
    static_contained_range_map_unittest
    static_map_unittest
    static_range_map_unittest
    synth_minidump_unittest)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  list(APPEND breakpad_processor_unittests fast_source_line_resolver_unittest)
endif()

# b/149265836
if(NOT WINDOWS_MSVC_X86_64)
  foreach(tst ${breakpad_processor_unittests})
    add_proc_test(${tst})
  endforeach()
endif()

android_add_executable(
  TARGET minidump_dump NODISTRIBUTE
  SRC # cmake-format: sortable
      ${BREAKPAD_SRC}/processor/minidump_dump.cc)
android_install_as_debug_info(minidump_dump)
set_property(TARGET minidump_dump PROPERTY CXX_STANDARD 17)
target_link_libraries(minidump_dump PRIVATE breakpad_processor)

android_add_executable(
  TARGET minidump_stackwalk NODISTRIBUTE
  SRC # cmake-format: sortable
      ${BREAKPAD_SRC}/processor/minidump_stackwalk.cc)
android_install_as_debug_info(minidump_stackwalk)
set_property(TARGET minidump_stackwalk PROPERTY CXX_STANDARD 17)
target_link_libraries(minidump_stackwalk PRIVATE breakpad_processor
                                                 breakpad_common)

#
# breakpad_server
android_add_library(
  TARGET breakpad_server
  LIBNAME
    breakpad
    URL
    "https://android.googlesource.com/platform/external/google-breakpad/+/refs/heads/emu-master-dev"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../google-breakpad"
  NOTICE "REPO/LICENSE"
  LICENSE "BSD-3-Clause"
  WINDOWS
    ${BREAKPAD_SRC}/client/windows/crash_generation/client_info.cc
    ${BREAKPAD_SRC}/client/windows/crash_generation/crash_generation_server.cc
  LINUX ${BREAKPAD_SRC}/client/linux/crash_generation/crash_generation_server.cc
  DARWIN ${BREAKPAD_SRC}/client/mac/crash_generation/crash_generation_server.cc)
set_property(TARGET breakpad_server PROPERTY CXX_STANDARD 17)
android_target_include_directories(breakpad_server linux-x86_64 PUBLIC
                                   ${ANDROID_QEMU2_TOP_DIR}/android)
android_target_include_directories(breakpad_server linux-aarch64 PUBLIC
                                   ${ANDROID_QEMU2_TOP_DIR}/android)
android_target_compile_definitions(breakpad_server windows PRIVATE
                                   ${ANDROID_BREAKPAD_WINDOWS_DEFS})
android_target_compile_definitions(breakpad_server linux-x86_64 PUBLIC
                                   -DHAVE_A_OUT_H)
android_target_compile_definitions(breakpad_server linux-aarch64 PUBLIC
                                   -DHAVE_A_OUT_H)
android_target_compile_definitions(breakpad_server darwin-x86_64 PUBLIC
                                   -DHAVE_MACH_O_NLIST_H)
target_link_libraries(breakpad_server PUBLIC breakpad_common breakpad_processor)

#
# breakpad_client
android_add_library(
  TARGET breakpad_client
  LIBNAME
    breakpad
    URL
    "https://android.googlesource.com/platform/external/google-breakpad/+/refs/heads/emu-master-dev"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../google-breakpad"
  NOTICE "REPO/LICENSE"
  LICENSE "BSD-3-Clause"
  LINUX ${BREAKPAD_SRC}/client/linux/crash_generation/crash_generation_client.cc
        ${BREAKPAD_SRC}/client/linux/crash_generation/crash_generation_server.cc
        ${BREAKPAD_SRC}/client/linux/dump_writer_common/thread_info.cc
        ${BREAKPAD_SRC}/client/linux/dump_writer_common/ucontext_reader.cc
        ${BREAKPAD_SRC}/client/linux/handler/exception_handler.cc
        ${BREAKPAD_SRC}/client/linux/handler/minidump_descriptor.cc
        ${BREAKPAD_SRC}/client/linux/log/log.cc
        ${BREAKPAD_SRC}/client/linux/microdump_writer/microdump_writer.cc
        ${BREAKPAD_SRC}/client/linux/minidump_writer/linux_core_dumper.cc
        ${BREAKPAD_SRC}/client/linux/minidump_writer/linux_dumper.cc
        ${BREAKPAD_SRC}/client/linux/minidump_writer/linux_ptrace_dumper.cc
        ${BREAKPAD_SRC}/client/linux/minidump_writer/minidump_writer.cc
        ${BREAKPAD_SRC}/client/minidump_file_writer.cc
  DARWIN ${BREAKPAD_SRC}/client/mac/crash_generation/ConfigFile.mm
         ${BREAKPAD_SRC}/client/mac/crash_generation/crash_generation_client.cc
         ${BREAKPAD_SRC}/client/mac/crash_generation/crash_generation_server.cc
         ${BREAKPAD_SRC}/client/mac/crash_generation/Inspector.mm
         ${BREAKPAD_SRC}/client/mac/crash_generation/InspectorMain.mm
         ${BREAKPAD_SRC}/client/mac/handler/breakpad_nlist_64.cc
         ${BREAKPAD_SRC}/client/mac/handler/dynamic_images.cc
         ${BREAKPAD_SRC}/client/mac/handler/exception_handler.cc
         ${BREAKPAD_SRC}/client/mac/handler/minidump_generator.cc
         ${BREAKPAD_SRC}/client/mac/handler/protected_memory_allocator.cc
         ${BREAKPAD_SRC}/client/minidump_file_writer.cc
  WINDOWS
    ${BREAKPAD_SRC}/client/windows/crash_generation/client_info.cc
    ${BREAKPAD_SRC}/client/windows/crash_generation/crash_generation_client.cc
    ${BREAKPAD_SRC}/client/windows/crash_generation/crash_generation_server.cc
    ${BREAKPAD_SRC}/client/windows/crash_generation/minidump_generator.cc
    ${BREAKPAD_SRC}/client/windows/handler/exception_handler.cc)
set_property(TARGET breakpad_client PROPERTY CXX_STANDARD 17)
target_include_directories(breakpad_client PUBLIC ${BREAKPAD_SRC})
android_target_include_directories(
  breakpad_client darwin-x86_64 PRIVATE ${BREAKPAD_SRC}/client/apple/Framework/
  ${BREAKPAD_SRC}/common/mac)
android_target_include_directories(breakpad_client linux-x86_64 PUBLIC
                                   ${ANDROID_QEMU2_TOP_DIR}/android)
android_target_include_directories(breakpad_client linux-aarch64 PUBLIC
                                   ${ANDROID_QEMU2_TOP_DIR}/android)
android_target_compile_definitions(breakpad_client windows PRIVATE
                                   ${ANDROID_BREAKPAD_WINDOWS_DEFS})
android_target_compile_definitions(breakpad_client linux-x86_64 PUBLIC
                                   -DHAVE_A_OUT_H)
android_target_compile_definitions(breakpad_client linux-aarch64 PUBLIC
                                   -DHAVE_A_OUT_H)
android_target_compile_definitions(
  breakpad_client darwin-x86_64 PRIVATE -DOBJC_OLD_DISPATCH_PROTOTYPES=0
  -DHAVE_MACH_O_NLIST_H)

target_link_libraries(breakpad_client PRIVATE breakpad_common
                                              breakpad_processor)

#
# dump_syms
if(NOT LINUX_AARCH64)
  android_add_executable(
    TARGET dump_syms
    NODISTRIBUTE
    WINDOWS ${BREAKPAD_SRC}/tools/windows/dump_syms/dump_syms.cc
    LINUX ${BREAKPAD_SRC}/tools/linux/dump_syms/dump_syms.cc
    DARWIN ${BREAKPAD_SRC}/common/mac/dump_syms.cc
           ${BREAKPAD_SRC}/tools/mac/dump_syms/dump_syms_tool.cc)
  android_install_as_debug_info(dump_syms)
  set_property(TARGET dump_syms PROPERTY CXX_STANDARD 17)
  target_link_libraries(dump_syms PRIVATE breakpad_common)
  android_target_link_libraries(dump_syms windows_msvc-x86_64
                                PRIVATE shell32::shell32 diaguids::diaguids)
  if(ANDROID_HOST_TAG STREQUAL "windows_msvc-x86_64")
    android_target_link_libraries(dump_syms windows_msvc-x86_64
                                  PRIVATE atls::atls)
  endif()
  android_target_compile_definitions(dump_syms linux-x86_64 PUBLIC
                                     -DHAVE_A_OUT_H)
  android_target_compile_definitions(dump_syms linux-aarch64 PUBLIC
                                     -DHAVE_A_OUT_H)
  android_target_compile_definitions(dump_syms darwin-x86_64 PUBLIC
                                     -DHAVE_MACH_O_NLIST_H)
  #
  # Symupload
  android_add_executable(
    TARGET sym_upload
    NODISTRIBUTE
    LINUX ${BREAKPAD_SRC}/common/linux/http_upload.cc
          ${BREAKPAD_SRC}/common/linux/symbol_upload.cc
          ${BREAKPAD_SRC}/tools/linux/symupload/sym_upload.cc
    DARWIN ${BREAKPAD_SRC}/tools/mac/symupload/symupload.m
    WINDOWS ${BREAKPAD_SRC}/tools/windows/symupload/symupload.cc)
  set_property(TARGET sym_upload PROPERTY CXX_STANDARD 17)
  android_target_include_directories(sym_upload darwin-x86_64 PRIVATE
                                     ${BREAKPAD_SRC}/common/mac/)
  target_link_libraries(sym_upload PRIVATE breakpad_common)
  # Note, sym_upload.exe will likely only work on the machine you are building
  # it on due to the mincore dependency. the mincore dependency will bring in
  # Api-ms- win-core-version-l1-1-0.dll which may not be available on every
  # windows version.
  android_target_link_libraries(
    sym_upload windows PRIVATE dbghelp::dbghelp diaguids::diaguids
                               mincore::mincore)
  android_target_link_libraries(sym_upload darwin-x86_64
                                PRIVATE "-framework Foundation")
  android_target_compile_definitions(sym_upload linux-x86_64 PUBLIC
                                     -DHAVE_A_OUT_H)
  android_target_compile_definitions(sym_upload linux-aarch64 PUBLIC
                                     -DHAVE_A_OUT_H)
  android_target_compile_definitions(sym_upload darwin-x86_64 PRIVATE
                                     -DOBJC_OLD_DISPATCH_PROTOTYPES=0)
  android_target_compile_definitions(sym_upload windows PRIVATE -DUNICODE
                                     -D_UNICODE)
  android_install_as_debug_info(sym_upload)
  if(LINUX_X86_64)
    android_add_executable(
      TARGET linux_dumper_unittest_helper
      NODISTRIBUTE
      SRC # cmake-format: sortable
          ${BREAKPAD_SRC}/client/linux/minidump_writer/linux_dumper_unittest_helper.cc
    )
    target_compile_options(linux_dumper_unittest_helper PRIVATE "-Wno-error")
    set_property(TARGET linux_dumper_unittest_helper PROPERTY CXX_STANDARD 17)
    target_link_libraries(
      linux_dumper_unittest_helper
      PRIVATE
        breakpad_common
        Threads::Threads
        "-Wl,-rpath,'${CMAKE_LIBRARY_OUTPUT_DIRECTORY}'"
        "-Wl,--build-id=0x000102030405060708090a0b0c0d0e0f101712131415161718191a1b1c1d1e1f"
    )

    # ASAN does not play nice with breakpad tests.
    if(NOT OPTION_ASAN)
      android_add_test(
        TARGET linux_client_unittests
        SRC # cmake-format: sortable
            ${BREAKPAD_SRC}/client/linux/handler/exception_handler_unittest.cc
            ${BREAKPAD_SRC}/client/linux/minidump_writer/cpu_set_unittest.cc
            ${BREAKPAD_SRC}/client/linux/minidump_writer/directory_reader_unittest.cc
            ${BREAKPAD_SRC}/client/linux/minidump_writer/line_reader_unittest.cc
            ${BREAKPAD_SRC}/client/linux/minidump_writer/linux_core_dumper.cc
            ${BREAKPAD_SRC}/client/linux/minidump_writer/linux_core_dumper_unittest.cc
            ${BREAKPAD_SRC}/client/linux/minidump_writer/linux_ptrace_dumper_unittest.cc
            # ${BREAKPAD_SRC}/client/linux/minidump_writer/minidump_writer_unittes
            # t.cc  # Ignore for now, we don't set proper ids.
            ${BREAKPAD_SRC}/client/linux/minidump_writer/minidump_writer_unittest_utils.cc
            ${BREAKPAD_SRC}/client/linux/minidump_writer/proc_cpuinfo_reader_unittest.cc
            ${BREAKPAD_SRC}/common/linux/elf_core_dump.cc
            ${BREAKPAD_SRC}/common/linux/linux_libc_support_unittest.cc
            ${BREAKPAD_SRC}/common/linux/tests/crash_generator.cc
            ${BREAKPAD_SRC}/common/memory_allocator_unittest.cc
            ${BREAKPAD_SRC}/common/tests/file_utils.cc
            ${BREAKPAD_SRC}/processor/basic_code_modules.cc
            ${BREAKPAD_SRC}/processor/convert_old_arm64_context.cc
            ${BREAKPAD_SRC}/processor/dump_context.cc
            ${BREAKPAD_SRC}/processor/dump_object.cc
            ${BREAKPAD_SRC}/processor/logging.cc
            ${BREAKPAD_SRC}/processor/pathname_stripper.cc
            ${BREAKPAD_SRC}/processor/proc_maps_linux.cc)
      android_target_include_directories(
        linux_client_unittests linux-x86_64 PUBLIC
        ${ANDROID_QEMU2_TOP_DIR}/android)
      target_link_libraries(linux_client_unittests PRIVATE breakpad_client
                                                           gmock_main)
      add_dependencies(linux_client_unittests linux_dumper_unittest_helper)
      add_proc_test(exploitability_unittest)
    endif()
  elseif(WINDOWS_MSVC_X86_64)
    android_add_test(
      TARGET windows_client_unittests
      SRC # cmake-format: sortable
          ${BREAKPAD_SRC}/client/windows/unittests/dump_analysis.cc
          # BUG: 151220246 this set of tests is flaky on windows
          # ${BREAKPAD_SRC}/client/windows/unittests/crash_generation_server_test.cc
          ${BREAKPAD_SRC}/client/windows/unittests/exception_handler_death_test.cc
          ${BREAKPAD_SRC}/client/windows/unittests/exception_handler_nesting_test.cc
          ${BREAKPAD_SRC}/client/windows/unittests/exception_handler_test.cc
          ${BREAKPAD_SRC}/client/windows/unittests/minidump_test.cc)
    target_link_libraries(windows_client_unittests
                          PRIVATE breakpad_client mincore::mincore gmock_main)
    android_target_compile_definitions(windows_client_unittests windows PRIVATE
                                       ${ANDROID_BREAKPAD_WINDOWS_DEFS})
  endif()
endif()
