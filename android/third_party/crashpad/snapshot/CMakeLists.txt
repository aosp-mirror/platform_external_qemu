enable_language(ASM)
android_add_library(
  TARGET crashpad_snapshot
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      annotation_snapshot.cc
      capture_memory.cc
      crashpad_info_client_options.cc
      handle_snapshot.cc
      memory_snapshot.cc
      minidump/exception_snapshot_minidump.cc
      minidump/memory_snapshot_minidump.cc
      minidump/minidump_annotation_reader.cc
      minidump/minidump_context_converter.cc
      minidump/minidump_simple_string_dictionary_reader.cc
      minidump/minidump_string_list_reader.cc
      minidump/minidump_string_reader.cc
      minidump/module_snapshot_minidump.cc
      minidump/process_snapshot_minidump.cc
      minidump/system_snapshot_minidump.cc
      minidump/thread_snapshot_minidump.cc
      unloaded_module_snapshot.cc
  LINUX # cmake-format: sortable
        crashpad_types/crashpad_info_reader.cc
        crashpad_types/image_annotation_reader.cc
        elf/elf_dynamic_array_reader.cc
        elf/elf_image_reader.cc
        elf/elf_symbol_table_reader.cc
        elf/module_snapshot_elf.cc
        linux/cpu_context_linux.cc
        linux/debug_rendezvous.cc
        linux/exception_snapshot_linux.cc
        linux/process_reader_linux.cc
        linux/process_snapshot_linux.cc
        linux/system_snapshot_linux.cc
        linux/thread_snapshot_linux.cc
        posix/timezone.cc
        sanitized/memory_snapshot_sanitized.cc
        sanitized/module_snapshot_sanitized.cc
        sanitized/process_snapshot_sanitized.cc
        sanitized/sanitization_information.cc
        sanitized/thread_snapshot_sanitized.cc
  DARWIN # cmake-format: sortable
         mac/cpu_context_mac.cc
         mac/exception_snapshot_mac.cc
         mac/mach_o_image_annotations_reader.cc
         mac/mach_o_image_reader.cc
         mac/mach_o_image_segment_reader.cc
         mac/mach_o_image_symbol_table_reader.cc
         mac/module_snapshot_mac.cc
         mac/process_reader_mac.cc
         mac/process_snapshot_mac.cc
         mac/process_types.cc
         mac/process_types/custom.cc
         mac/system_snapshot_mac.cc
         mac/thread_snapshot_mac.cc
         posix/timezone.cc
  WINDOWS # cmake-format: sortable
          crashpad_types/crashpad_info_reader.cc
          win/capture_memory_delegate_win.cc
          win/cpu_context_win.cc
          win/exception_snapshot_win.cc
          win/memory_map_region_snapshot_win.cc
          win/module_snapshot_win.cc
          win/pe_image_annotations_reader.cc
          win/pe_image_reader.cc
          win/pe_image_resource_reader.cc
          win/process_reader_win.cc
          win/process_snapshot_win.cc
          win/process_subrange_reader.cc
          win/system_snapshot_win.cc
          win/thread_snapshot_win.cc
          win/thread_snapshot_win.h)

if(LINUX_X86_64 OR WINDOWS OR DARWIN_X86_64)
  target_sources(crashpad_snapshot PRIVATE x86/cpuid_reader.cc)
endif()
android_target_link_libraries(crashpad_snapshot windows
                              PUBLIC powrprof::powrprof)
target_link_libraries(crashpad_snapshot PUBLIC mini_chromium crashpad_compat
                                               crashpad_util crashpad_context)

android_add_library(
  TARGET crashpad_context LICENSE Apache-2.0 SRC # cmake-format: sortable
                                                 cpu_context.cc)

target_link_libraries(crashpad_context PUBLIC crashpad_util)

android_add_library(
  TARGET crashpad_snapshot_test_support
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      test/test_cpu_context.cc
      test/test_exception_snapshot.cc
      test/test_memory_map_region_snapshot.cc
      test/test_memory_snapshot.cc
      test/test_module_snapshot.cc
      test/test_process_snapshot.cc
      test/test_system_snapshot.cc
      test/test_thread_snapshot.cc)
target_link_libraries(crashpad_snapshot_test_support PUBLIC crashpad_snapshot)

if(WINDOWS_MSVC_X86_64)
  # Some odd config with timezones  on our windows buildbots.
  set(SNAPSHOT_TEST_FILTER "--gtest_filter=-SystemSnapshotWinTest.TimeZone")
endif()

if(LINUX_X86_64)
  # There is some bizarre configuration issue on our bots, preventing us from
  # setting thread priorities
  set(SNAPSHOT_TEST_FILTER "--gtest_filter=-*Threads*")
endif()

if(OPTION_ASAN)
  message(STATUS "Disabling snapshot tests for asan builds")
else()
  android_add_test(
    TARGET crashpad_snapshot_test
    SRC # cmake-format: sortable
        cpu_context_test.cc memory_snapshot_test.cc
        minidump/process_snapshot_minidump_test.cc
    LINUX # cmake-format: sortable
          crashpad_types/crashpad_info_reader_test.cc
          crashpad_types/image_annotation_reader_test.cc
          elf/elf_image_reader_test.cc
          elf/elf_image_reader_test_note.S
          linux/debug_rendezvous_test.cc
          linux/exception_snapshot_linux_test.cc
          linux/process_reader_linux_test.cc
          linux/system_snapshot_linux_test.cc
          linux/test_modules.cc
          posix/timezone_test.cc
          sanitized/process_snapshot_sanitized_test.cc
          sanitized/sanitization_information_test.cc
    DARWIN # cmake-format: sortable
           mac/cpu_context_mac_test.cc
           mac/mach_o_image_annotations_reader_test.cc
           mac/mach_o_image_reader_test.cc
           mac/mach_o_image_segment_reader_test.cc
           mac/process_reader_mac_test.cc
           mac/process_types_test.cc
           mac/system_snapshot_mac_test.cc
           posix/timezone_test.cc
    WINDOWS # cmake-format: sortable
            crashpad_types/crashpad_info_reader_test.cc
            win/cpu_context_win_test.cc
            win/exception_snapshot_win_test.cc
            win/extra_memory_ranges_test.cc
            win/module_snapshot_win_test.cc
            win/pe_image_reader_test.cc
            win/process_reader_win_test.cc
            win/process_snapshot_win_test.cc
            win/system_snapshot_win_test.cc
    TEST_PARAMS "${SNAPSHOT_TEST_FILTER}")
  target_link_libraries(
    crashpad_snapshot_test
    PRIVATE crashpad_test
            crashpad_client_common
            crashpad_context
            crashpad_snapshot_test_support
            crashpad_compat
            crashpad_util
            gmock
            crashpad_googlemock_main)
  android_target_link_libraries(crashpad_snapshot_test darwin-x86_64
                                PRIVATE "-framework OpenCL")
  android_target_link_libraries(crashpad_snapshot_test darwin-aarch64
                                PRIVATE "-framework OpenCL")
  android_target_link_libraries(
    crashpad_snapshot_test linux
    PRIVATE
      "-Wl,--dynamic-list,${CMAKE_CURRENT_LIST_DIR}/elf/test_exported_symbols.sym"
  )

  set_property(
    TEST crashpad_snapshot_test APPEND
    PROPERTY ENVIRONMENT "CRASHPAD_TEST_DATA_ROOT=${CRASHPAD_TEST_DATA_ROOT}")
endif()

if(DARWIN_X86_64 OR DARWIN_AARCH64)
  android_add_library(
    TARGET crashpad_snapshot_test_module_crashy_initializer SHARED NODISTRIBUTE
    SRC mac/mach_o_image_annotations_reader_test_module_crashy_initializer.cc)
  set_target_properties(
    crashpad_snapshot_test_module_crashy_initializer
    PROPERTIES SUFFIX ".so" PREFIX "" LIBRARY_OUTPUT_DIRECTORY
                                      "${CMAKE_BINARY_DIR}")
  android_add_executable(TARGET crashpad_snapshot_test_no_op NODISTRIBUTE
                         SRC mac/mach_o_image_annotations_reader_test_no_op.cc)
  add_dependencies(
    crashpad_snapshot_test crashpad_snapshot_test_module_crashy_initializer
    crashpad_snapshot_test_no_op)
endif()

android_add_library(
  TARGET crashpad_snapshot_test_module
  LICENSE Apache-2.0
  SHARED
  SRC # cmake-format: sortable
      crashpad_info_client_options_test_module.cc
  LINUX # cmake-format: sortable
  DARWIN # cmake-format: sortable
  WINDOWS # cmake-format: sortable
)

target_link_libraries(crashpad_snapshot_test_module PRIVATE mini_chromium
                                                            crashpad_client)
set_target_properties(
  crashpad_snapshot_test_module
  PROPERTIES SUFFIX ".so" PREFIX "" LIBRARY_OUTPUT_DIRECTORY
                                    "${CMAKE_BINARY_DIR}")

android_add_library(
  TARGET crashpad_snapshot_test_module_large
  LICENSE Apache-2.0
  SHARED
  SRC # cmake-format: sortable
      crashpad_info_size_test_module.cc
  LINUX # cmake-format: sortable
        crashpad_info_size_test_note.S
  DARWIN # cmake-format: sortable
  WINDOWS # cmake-format: sortable
)

target_compile_definitions(crashpad_snapshot_test_module_large
                           PRIVATE CRASHPAD_INFO_SIZE_TEST_MODULE_LARGE)
target_link_libraries(crashpad_snapshot_test_module_large PRIVATE mini_chromium
                                                                  crashpad_util)
set_target_properties(
  crashpad_snapshot_test_module_large
  PROPERTIES SUFFIX ".so" PREFIX "" LIBRARY_OUTPUT_DIRECTORY
                                    "${CMAKE_BINARY_DIR}")

android_add_library(
  TARGET crashpad_snapshot_test_module_small
  LICENSE Apache-2.0
  SHARED
  SRC # cmake-format: sortable
      crashpad_info_size_test_module.cc
  LINUX # cmake-format: sortable
        crashpad_info_size_test_note.S
  DARWIN # cmake-format: sortable
  WINDOWS # cmake-format: sortable
)

target_compile_definitions(crashpad_snapshot_test_module_small
                           PRIVATE CRASHPAD_INFO_SIZE_TEST_MODULE_SMALL)
target_link_libraries(crashpad_snapshot_test_module_small PRIVATE mini_chromium
                                                                  crashpad_util)
set_target_properties(
  crashpad_snapshot_test_module_small
  PROPERTIES SUFFIX ".so" PREFIX "" LIBRARY_OUTPUT_DIRECTORY
                                    "${CMAKE_BINARY_DIR}")
if(LINUX_X86_64 OR LINUX_AARCH64)

  android_add_library(
    TARGET crashpad_snapshot_test_both_dt_hash_styles LICENSE Apache-2.0 SHARED
    SRC # cmake-format: sortable
        hash_types_test.cc)

  target_link_libraries(
    crashpad_snapshot_test_both_dt_hash_styles
    PRIVATE # This makes `ld` emit both .hash and .gnu.hash sections.
            "-Wl,--hash-style=both")
  set_target_properties(
    crashpad_snapshot_test_both_dt_hash_styles
    PROPERTIES SUFFIX ".so" PREFIX "" LIBRARY_OUTPUT_DIRECTORY
                                      "${CMAKE_BINARY_DIR}")
endif()

if(WINDOWS_MSVC_X86_64)

  android_add_executable(TARGET crashpad_snapshot_test_annotations NODISTRIBUTE
                         SRC win/crashpad_snapshot_test_annotations.cc)
  target_link_libraries(crashpad_snapshot_test_annotations
                        PRIVATE mini_chromium crashpad_client crashpad_compat)

  android_add_executable(
    TARGET crashpad_snapshot_test_crashing_child NODISTRIBUTE
    SRC win/crashpad_snapshot_test_crashing_child.cc)
  target_link_libraries(
    crashpad_snapshot_test_crashing_child PRIVATE mini_chromium crashpad_client
                                                  crashpad_compat crashpad_util)

  android_add_executable(
    TARGET crashpad_snapshot_test_dump_without_crashing NODISTRIBUTE
    SRC win/crashpad_snapshot_test_dump_without_crashing.cc)
  target_link_libraries(
    crashpad_snapshot_test_dump_without_crashing
    PRIVATE mini_chromium crashpad_client crashpad_compat crashpad_util)

  android_add_executable(
    TARGET crashpad_snapshot_test_extra_memory_ranges NODISTRIBUTE
    SRC win/crashpad_snapshot_test_extra_memory_ranges.cc)
  target_link_libraries(crashpad_snapshot_test_extra_memory_ranges
                        PRIVATE mini_chromium crashpad_client crashpad_compat)

  android_add_executable(TARGET crashpad_snapshot_test_image_reader NODISTRIBUTE
                         SRC win/crashpad_snapshot_test_image_reader.cc)
  target_link_libraries(
    crashpad_snapshot_test_image_reader PRIVATE mini_chromium crashpad_client
                                                crashpad_compat crashpad_util)

  add_library(crashpad_snapshot_test_image_reader_module SHARED
              win/crashpad_snapshot_test_image_reader_module.cc)
  target_link_libraries(crashpad_snapshot_test_image_reader_module
                        PRIVATE mini_chromium crashpad_client)
  set_target_properties(
    crashpad_snapshot_test_image_reader_module
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}" PREFIX "")
endif()
