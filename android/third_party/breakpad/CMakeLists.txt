cmake_minimum_required(VERSION 3.23)
project(breakpad)

if(NOT ANDROID_QEMU2_TOP_DIR)
  # You are likely including this from outside qemu.
  # TODO(jansene): We should just move this to breakpad itself.
  get_filename_component(ANDROID_QEMU2_TOP_DIR
                         "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)
  get_filename_component(
    ADD_PATH "${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/" ABSOLUTE)
  list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")

  include(android)
  prebuilt(THREADS)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)

endif()

if(NOT TARGET gmock_main)
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/android/third_party/googletest
                   googletest)
endif()
# Location of the root of the breakpad source.
set(BREAKPAD_SRC ${ANDROID_QEMU2_TOP_DIR}/../google-breakpad/src)

android_add_interface(TARGET breakpad_compiler_config LICENSE None)
if(APPLE)
  target_compile_definitions(
    breakpad_compiler_config
    INTERFACE HAVE_ARC4RANDOM
              HAVE_CXX7
              HAVE_GETCONTEXT
              HAVE_INTTYPES_H
              HAVE_PTHREAD
              HAVE_STDINT_H
              HAVE_STDIO_H
              HAVE_STDLIB_H
              HAVE_STRING_H
              HAVE_STRINGS_H
              HAVE_SYS_MMAN_H
              HAVE_SYS_RANDOM_H
              HAVE_SYS_STAT_H
              HAVE_SYS_TYPES_H
              HAVE_UNISTD_H
              HAVE_MACH_O_NLIST_H
              _THREAD_SAFE
              STDC_HEADERS)
  target_compile_options(
    breakpad_compiler_config
    INTERFACE -Wmissing-braces
              -Wnon-virtual-dtor
              -Woverloaded-virtual
              -Wreorder
              -Wsign-compare
              -Wunused-local-typedefs
              -Wunused-variable
              -Wvla
              -UNDEBUG
              -Wno-error)
elseif(LINUX)
  target_compile_definitions(
    breakpad_compiler_config
    INTERFACE HAVE_A_OUT_H
              HAVE_CXX17
              HAVE_GETCONTEXT
              HAVE_GETRANDOM
              HAVE_INTTYPES_H
              HAVE_MEMFD_CREATE
              HAVE_PTHREAD
              HAVE_STDINT_H
              HAVE_STDIO_H
              HAVE_STDLIB_H
              HAVE_STRINGS_H
              HAVE_STRING_H
              HAVE_SYS_MMAN_H
              HAVE_SYS_STAT_H
              HAVE_SYS_TY
              # Older glibc elf.h might not yet define the ELF compression
              # types.
              SHF_COMPRESSED=2048)
  target_compile_options(breakpad_compiler_config INTERFACE -fPIC -g -UNDEBUG
                                                            -Wno-fortify-source)
else()
  target_compile_definitions(
    breakpad_compiler_config
    INTERFACE _CRT_NONSTDC_NO_DEPRECATE
              _CRT_NONSTDC_NO_WARNINGS
              _CRT_RAND_S
              _CRT_SECURE_NO_DEPRECATE
              _HAS_EXCEPTIONS=0
              _SECURE_ATL
              _UNICODE
              _WIN32_WINNT=0x0600
              _WINDOWS
              NOMINMAX
              UNICODE
              WIN32
              WIN32_LEAN_AND_MEAN
              WINVER=0x0600)

  target_compile_options(breakpad_compiler_config INTERFACE -g -UNDEBUG)
endif()

android_add_library(
  TARGET breakpad_disasm
  LIBNAME
    breakpad
    URL
    "https://android.googlesource.com/platform/external/google-breakpad/+/refs/heads/emu-master-dev"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../google-breakpad"
  NOTICE "REPO/LICENSE"
  LICENSE "BSD-3-Clause"
  SOURCE_DIR ${BREAKPAD_SRC}
  SRC third_party/libdisasm/ia32_implicit.c
      third_party/libdisasm/ia32_insn.c
      third_party/libdisasm/ia32_invariant.c
      third_party/libdisasm/ia32_modrm.c
      third_party/libdisasm/ia32_opcode_tables.c
      third_party/libdisasm/ia32_operand.c
      third_party/libdisasm/ia32_reg.c
      third_party/libdisasm/ia32_settings.c
      third_party/libdisasm/x86_disasm.c
      third_party/libdisasm/x86_format.c
      third_party/libdisasm/x86_imm.c
      third_party/libdisasm/x86_insn.c
      third_party/libdisasm/x86_misc.c
      third_party/libdisasm/x86_operand_list.c)

target_compile_features(breakpad_disasm PRIVATE cxx_std_17)
android_target_compile_options(breakpad_disasm windows
                               PRIVATE -Wno-int-to-void-pointer-cast)
target_link_libraries(breakpad_disasm PRIVATE breakpad_compiler_config)

android_add_library(
  TARGET breakpad_common
  LIBNAME
    breakpad
    URL
    "https://android.googlesource.com/platform/external/google-breakpad/+/refs/heads/emu-master-dev"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../google-breakpad"
  NOTICE "REPO/LICENSE"
  LICENSE "BSD-3-Clause"
  SOURCE_DIR ${BREAKPAD_SRC}
  SRC common/convert_UTF.cc
      common/long_string_dictionary.cc
      common/md5.cc
      common/module.cc
      common/simple_string_dictionary.cc
      common/string_conversion.cc
      common/test_assembler.cc
  WINDOWS common/windows/dia_util.cc
          common/windows/guid_string.cc
          common/windows/http_upload.cc
          common/windows/omap.cc
          common/windows/pdb_source_line_writer.cc
          common/windows/pe_source_line_writer.cc
          common/windows/pe_util.cc
          common/windows/string_utils.cc
          common/windows/sym_upload_v2_protocol.cc
          common/windows/symbol_collector_client.cc
  LINUX common/dwarf/bytereader.cc
        common/dwarf/cfi_assembler.cc
        common/dwarf/dwarf2diehandler.cc
        common/dwarf/dwarf2reader.cc
        common/dwarf/elf_reader.cc
        common/dwarf_cfi_to_module.cc
        common/dwarf_cu_to_module.cc
        common/dwarf_line_to_module.cc
        common/dwarf_range_list_handler.cc
        common/language.cc
        common/linux/crc32.cc
        common/linux/dump_symbols.cc
        common/linux/elf_core_dump.cc
        common/linux/elf_symbols_to_module.cc
        common/linux/elfutils.cc
        common/linux/file_id.cc
        common/linux/google_crashdump_uploader.cc
        common/linux/guid_creator.cc
        common/linux/http_upload.cc
        common/linux/libcurl_wrapper.cc
        common/linux/linux_libc_support.cc
        common/linux/memory_mapped_file.cc
        common/linux/safe_readlink.cc
        common/linux/synth_elf.cc
        common/path_helper.cc
        common/stabs_reader.cc
        common/stabs_to_module.cc
  DARWIN common/dwarf/bytereader.cc
         common/dwarf/cfi_assembler.cc
         common/dwarf/dwarf2diehandler.cc
         common/dwarf/dwarf2reader.cc
         common/dwarf/elf_reader.cc
         common/dwarf_cfi_to_module.cc
         common/dwarf_cu_to_module.cc
         common/dwarf_line_to_module.cc
         common/dwarf_range_list_handler.cc
         common/language.cc
         common/mac/arch_utilities.cc
         common/mac/bootstrap_compat.cc
         common/mac/dump_syms.cc
         common/mac/file_id.cc
         common/mac/GTMLogger.m
         common/mac/MachIPC.mm
         common/mac/macho_id.cc
         common/mac/macho_reader.cc
         common/mac/macho_utilities.cc
         common/mac/macho_walker.cc
         common/mac/string_utilities.cc
         common/path_helper.cc
         common/stabs_reader.cc
         common/stabs_to_module.cc)
target_compile_features(breakpad_common PRIVATE cxx_std_17)
target_include_directories(breakpad_common PUBLIC ${BREAKPAD_SRC})
target_link_libraries(breakpad_common PRIVATE breakpad_compiler_config zlib)

android_target_link_libraries2(
  TARGET breakpad_common DARWIN PRIVATE "-framework Foundation"
                                        "-framework CoreFoundation")

if(LINUX)
  target_include_directories(breakpad_common
                             PUBLIC ${ANDROID_QEMU2_TOP_DIR}/android)
  target_link_libraries(breakpad_common PRIVATE dl)
  # We do not have the linux headers that specify EM_RISCV, see:
  # https://elixir.bootlin.com/linux/latest/source/include/uapi/linux/elf-em.h#L51
  # For the proper kernel definitions.
  target_compile_definitions(breakpad_common PRIVATE EM_RISCV=243)
endif()

android_target_link_libraries2(
  TARGET breakpad_common WINDOWS
  PRIVATE wininet::wininet diaguids::diaguids atls::atls dbghelp::dbghelp
          imagehlp::imagehlp)

# ASAN does not play nice with breakpad tests
if(NOT OPTION_ASAN)
  android_add_test(
    TARGET breakpad_common_unittests
    SOURCE_DIR ${BREAKPAD_SRC}
    SRC common/byte_cursor_unittest.cc
        common/long_string_dictionary_unittest.cc
        common/memory_range_unittest.cc
        common/module_unittest.cc
        common/simple_string_dictionary_unittest.cc
        # common/test_assembler_unittest.cc
    WINDOWS common/windows/omap_unittest.cc
    DARWIN common/dwarf/bytereader_unittest.cc
           common/dwarf/dwarf2reader_die_unittest.cc
           common/dwarf_cu_to_module_unittest.cc
           common/dwarf_line_to_module_unittest.cc
           common/memory_allocator_unittest.cc
           # common/mac/macho_reader_unittest.cc # Some gmock issues, these fail
           # in xcode as well common/dwarf/dwarf2reader_cfi_unittest.cc
           # common/stabs_to_module_unittest.cc
    LINUX common/dwarf/bytereader_unittest.cc
          # common/dwarf/dwarf2diehandler_unittest.cc
          common/dwarf/dwarf2reader_cfi_unittest.cc
          # common/dwarf/dwarf2reader_die_unittest.cc
          common/dwarf_cfi_to_module_unittest.cc
          common/dwarf_cu_to_module_unittest.cc
          common/dwarf_line_to_module_unittest.cc
          common/linux/dump_symbols_unittest.cc
          common/linux/elf_symbols_to_module_unittest.cc
          common/linux/file_id_unittest.cc
          common/linux/google_crashdump_uploader_test.cc
          common/linux/linux_libc_support_unittest.cc
          common/linux/memory_mapped_file_unittest.cc
          common/linux/safe_readlink_unittest.cc
          common/linux/synth_elf_unittest.cc
          common/linux/tests/crash_generator.cc
          common/memory_allocator_unittest.cc
          common/stabs_reader_unittest.cc
          common/stabs_to_module_unittest.cc
          common/tests/file_utils.cc)
  target_link_libraries(
    breakpad_common_unittests PRIVATE breakpad_common gmock_main
                                      breakpad_compiler_config)
  android_target_link_libraries2(TARGET breakpad_common_unittests WINDOWS
                                 PRIVATE diaguids::diaguids)
endif()

android_add_library(
  TARGET breakpad_processor
  LIBNAME
    breakpad
    URL
    "https://android.googlesource.com/platform/external/google-breakpad/+/refs/heads/emu-master-dev"
  REPO "${ANDROID_QEMU2_TOP_DIR}/../google-breakpad"
  NOTICE "REPO/LICENSE"
  LICENSE "BSD-3-Clause"
  SOURCE_DIR ${BREAKPAD_SRC}
  SRC processor/basic_code_modules.cc
      processor/basic_source_line_resolver.cc
      processor/call_stack.cc
      processor/cfi_frame_info.cc
      processor/convert_old_arm64_context.cc
      processor/disassembler_x86.cc
      processor/dump_context.cc
      processor/dump_object.cc
      processor/exploitability.cc
      processor/exploitability_linux.cc
      processor/exploitability_win.cc
      processor/fast_source_line_resolver.cc
      processor/logging.cc
      processor/microdump.cc
      processor/microdump_processor.cc
      processor/minidump.cc
      processor/minidump_processor.cc
      processor/module_comparer.cc
      processor/module_serializer.cc
      processor/pathname_stripper.cc
      processor/proc_maps_linux.cc
      processor/process_state.cc
      processor/simple_symbol_supplier.cc
      processor/source_line_resolver_base.cc
      processor/stack_frame_cpu.cc
      processor/stack_frame_symbolizer.cc
      processor/stackwalk_common.cc
      processor/stackwalker.cc
      processor/stackwalker_address_list.cc
      processor/stackwalker_amd64.cc
      processor/stackwalker_arm.cc
      processor/stackwalker_arm64.cc
      processor/stackwalker_mips.cc
      processor/stackwalker_ppc.cc
      processor/stackwalker_ppc64.cc
      processor/stackwalker_riscv.cc
      processor/stackwalker_riscv64.cc
      processor/stackwalker_sparc.cc
      processor/stackwalker_x86.cc
      processor/symbolic_constants_win.cc
      processor/synth_minidump.cc
      processor/tokenize.cc
  LINUX common/linux/scoped_pipe.cc common/linux/scoped_tmpfile.cc
        processor/disassembler_objdump.cc processor/minidump.cc)
set_property(TARGET breakpad_processor PROPERTY CXX_STANDARD 17)
target_include_directories(breakpad_processor PUBLIC ${BREAKPAD_SRC})
target_link_libraries(
  breakpad_processor PRIVATE breakpad_disasm breakpad_common
                             breakpad_compiler_config)
target_compile_definitions(breakpad_processor
                           PRIVATE BPLOG_MINIMUM_SEVERITY=SEVERITY_CRITICAL)
android_target_compile_definitions(
  breakpad_processor linux PRIVATE "O_TMPFILE=(020000000 | O_DIRECTORY)")

function(add_proc_test name)
  if(CROSSCOMPILE)
    return()
  endif()
  android_add_test(TARGET ${name} SRC ${BREAKPAD_SRC}/processor/${name}.cc)
  target_link_libraries(${name} PRIVATE breakpad_processor gmock_main
                                        breakpad_compiler_config)
  android_target_compile_definitions(${name} windows PRIVATE UNICODE _UNICODE)
  set_property(TEST ${name} PROPERTY WORKING_DIRECTORY "${BREAKPAD_SRC}/..")
endfunction()

set(breakpad_processor_unittests
    basic_source_line_resolver_unittest
    cfi_frame_info_unittest
    disassembler_x86_unittest
    fast_source_line_resolver_unittest
    map_serializers_unittest
    microdump_processor_unittest
    # minidump_processor_unittest # Does not work on linux
    minidump_unittest
    range_map_truncate_upper_unittest
    stackwalker_address_list_unittest
    stackwalker_amd64_unittest
    stackwalker_arm64_unittest
    stackwalker_arm_unittest
    stackwalker_x86_unittest
    static_address_map_unittest
    static_contained_range_map_unittest
    static_map_unittest
    static_range_map_unittest
    synth_minidump_unittest)

# b/149265836
if(NOT WINDOWS_MSVC_X86_64 AND NOT IN_CROSS_COMPILE)
  foreach(tst ${breakpad_processor_unittests})
    add_proc_test(${tst})
  endforeach()
endif()

if(NOT WINDOWS_MSVC_X86_64 AND NOT IN_CROSS_COMPILE)
  android_add_executable(TARGET minidump_dump NODISTRIBUTE
                         SRC ${BREAKPAD_SRC}/processor/minidump_dump.cc)
  set_property(TARGET minidump_dump PROPERTY CXX_STANDARD 17)
  target_link_libraries(minidump_dump PRIVATE breakpad_processor)

  android_add_executable(TARGET minidump_stackwalk NODISTRIBUTE
                         SRC ${BREAKPAD_SRC}/processor/minidump_stackwalk.cc)
  set_property(TARGET minidump_stackwalk PROPERTY CXX_STANDARD 17)
  target_link_libraries(minidump_stackwalk PRIVATE breakpad_processor
                                                   breakpad_common)
endif()

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
  android_target_link_libraries2(
    TARGET dump_syms WINDOWS PRIVATE shell32::shell32 diaguids::diaguids
                                     atls::atls)

  #
  # Symupload
  android_add_executable(
    TARGET sym_upload
    NODISTRIBUTE SOURCE_DIR ${BREAKPAD_SRC}
    LINUX common/linux/http_upload.cc common/linux/symbol_collector_client.cc
          common/linux/symbol_upload.cc common/path_helper.cc
          tools/linux/symupload/sym_upload.cc
    DARWIN common/mac/encoding_util.m
           common/mac/HTTPGetRequest.m
           common/mac/HTTPMultipartUpload.m
           common/mac/HTTPPutRequest.m
           common/mac/HTTPRequest.m
           common/mac/HTTPSimplePostRequest.m
           common/mac/SymbolCollectorClient.m
           tools/mac/symupload/symupload.mm
    WINDOWS tools/windows/symupload/symupload.cc)
  set_property(TARGET sym_upload PROPERTY CXX_STANDARD 17)
  android_target_include_directories(sym_upload darwin PRIVATE
                                     ${BREAKPAD_SRC}/common/mac/)
  target_link_libraries(sym_upload PRIVATE breakpad_common
                                           breakpad_compiler_config)
  # Note, sym_upload.exe will likely only work on the machine you are building
  # it on due to the mincore dependency. the mincore dependency will bring in
  # Api-ms- win-core-version-l1-1-0.dll which may not be available on every
  # windows version.
  android_target_link_libraries2(
    TARGET sym_upload WINDOWS PRIVATE dbghelp::dbghelp diaguids::diaguids
                                      mincore::mincore)
  android_target_link_libraries2(
    TARGET sym_upload DARWIN PRIVATE "-framework Foundation"
                                     "-framework CoreFoundation")
endif()
