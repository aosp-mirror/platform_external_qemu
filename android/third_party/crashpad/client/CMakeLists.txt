enable_language(ASM)
android_add_library(
  TARGET crashpad_client_common
  LICENSE Apache-2.0
  SRC annotation.cc annotation_list.cc crash_report_database.cc crashpad_info.cc
      settings.cc
  LINUX crash_report_database_generic.cc crashpad_info_note.S
        pthread_create_linux.cc
  DARWIN crash_report_database_mac.mm
  WINDOWS crash_report_database_win.cc)
target_compile_definitions(crashpad_client_common
                           PUBLIC CRASHPAD_FLOCK_ALWAYS_SUPPORTED=true)
target_compile_options(crashpad_client_common PRIVATE -Wno-missing-field-initializers)

android_target_compile_definitions(
  crashpad_client_common windows
  PRIVATE NOMINMAX
          UNICODE
          WIN32_LEAN_AND_MEAN
          _CRT_SECURE_NO_WARNINGS
          CRASHPAD_ZLIB_SOURCE_EMBEDDED
          _HAS_EXCEPTIONS=0
          _UNICODE)
target_link_libraries(crashpad_client_common PUBLIC crashpad_util)
android_add_library(
  TARGET crashpad_client
  LICENSE Apache-2.0
  SRC prune_crash_reports.cc
  LINUX client_argv_handling.cc crashpad_client_linux.cc
  DARWIN crashpad_client_mac.cc simulate_crash_mac.cc
  WINDOWS crashpad_client_win.cc)

target_compile_options(crashpad_client PRIVATE -Wno-missing-field-initializers)
target_link_libraries(crashpad_client PUBLIC crashpad_client_common)
android_target_compile_definitions(crashpad_client windows PRIVATE UNICODE)
target_compile_options(crashpad_client PRIVATE -Wno-missing-field-initializers)
android_target_link_libraries(crashpad_client_common linux-aarch64
                              PRIVATE crashpad_client)
android_add_test(
  TARGET crashpad_client_test
  SRC annotation_list_test.cc
      annotation_test.cc
      crash_report_database_test.cc
      length_delimited_ring_buffer_test.cc
      prune_crash_reports_test.cc
      settings_test.cc
      simple_address_range_bag_test.cc
      simple_string_dictionary_test.cc
  LINUX crashpad_client_linux_test.cc
  DARWIN simulate_crash_mac_test.cc
  WINDOWS crashpad_client_win_test.cc)

target_compile_options(crashpad_client_test PRIVATE -UNDEBUG)
target_link_libraries(
  crashpad_client_test PRIVATE crashpad_client crashpad_test gmock
                               crashpad_googlemock_main)
