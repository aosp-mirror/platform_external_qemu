enable_language(ASM)
android_add_library(
  TARGET crashpad_client_common
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      annotation.cc annotation_list.cc crash_report_database.cc crashpad_info.cc
      settings.cc
  LINUX # cmake-format: sortable
        crash_report_database_generic.cc crashpad_info_note.S
        pthread_create_linux.cc
  DARWIN # cmake-format: sortable
         crash_report_database_mac.mm
  WINDOWS # cmake-format: sortable
          crash_report_database_win.cc)

target_link_libraries(crashpad_client_common PUBLIC crashpad_util)
android_add_library(
  TARGET crashpad_client
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      prune_crash_reports.cc
  LINUX # cmake-format: sortable
        client_argv_handling.cc crashpad_client_linux.cc
  DARWIN # cmake-format: sortable
         crashpad_client_mac.cc simulate_crash_mac.cc
  WINDOWS # cmake-format: sortable
          crashpad_client_win.cc)

target_link_libraries(crashpad_client PUBLIC crashpad_client_common)
android_target_link_libraries(crashpad_client_common linux-aarch64 PRIVATE crashpad_client)
android_add_test(
  TARGET crashpad_client_test
  SRC # cmake-format: sortable
      annotation_list_test.cc
      annotation_test.cc
      crash_report_database_test.cc
      prune_crash_reports_test.cc
      settings_test.cc
      simple_address_range_bag_test.cc
      simple_string_dictionary_test.cc
  LINUX # cmake-format: sortable
        crashpad_client_linux_test.cc
  DARWIN # cmake-format: sortable
         simulate_crash_mac_test.cc
  WINDOWS # cmake-format: sortable
          crashpad_client_win_test.cc)
target_link_libraries(crashpad_client_test PRIVATE crashpad_client crashpad_test gmock crashpad_googlemock_main)
