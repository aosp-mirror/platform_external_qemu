android_add_library(
  TARGET crashpad_handler_handler
  LICENSE Apache-2.0
  SRC handler_main.cc prune_crash_reports_thread.cc user_stream_data_source.cc
  LINUX linux/capture_snapshot.cc
        linux/capture_snapshot.h
        linux/crash_report_exception_handler.cc
        linux/cros_crash_report_exception_handler.cc
        linux/exception_handler_server.cc
  DARWIN mac/crash_report_exception_handler.cc mac/exception_handler_server.cc
  WINDOWS win/crash_report_exception_handler.cc)

android_add_library(
  TARGET crashpad_handler_handler_common LICENSE Apache-2.0
  SRC crash_report_upload_thread.cc minidump_to_upload_parameters.cc LINUX
  DARWIN mac/file_limit_annotation.cc WINDOWS)
target_link_libraries(
  crashpad_handler_handler_common PUBLIC crashpad_util crashpad_compat
                                         crashpad_client_common)

target_link_libraries(
  crashpad_handler_handler
  PUBLIC crashpad_util crashpad_compat crashpad_handler_handler_common
         crashpad_tool_support crashpad_client crashpad_minidump)
android_add_executable(TARGET crashpad_handler LICENSE Apache-2.0 SRC main.cc
                       LINUX DARWIN WINDOWS)
target_link_libraries(crashpad_handler PRIVATE crashpad_handler_handler
                                               Threads::Threads)

install(TARGETS crashpad_handler RUNTIME DESTINATION .)
android_sign(
  INSTALL ${CMAKE_INSTALL_PREFIX}/crashpad_handler${CMAKE_EXECUTABLE_SUFFIX})

if(OPTION_ASAN)
  message(STATUS "Disabling snapshot tests for asan builds")
else()
  android_add_test(
    TARGET crashpad_handler_test SRC minidump_to_upload_parameters_test.cc LINUX
    DARWIN WINDOWS crashpad_handler_test.cc)
  target_link_libraries(
    crashpad_handler_test
    PRIVATE crashpad_handler_handler crashpad_snapshot
            crashpad_snapshot_test_support gmock crashpad_googlemock_main)
endif()

android_add_executable(
  TARGET crashpad_handler_test_extended_handler NODISTRIBUTE
  SRC crashpad_handler_test_extended_handler.cc LINUX DARWIN WINDOWS)
target_link_libraries(
  crashpad_handler_test_extended_handler
  PRIVATE crashpad_handler_handler crashpad_compat
          crashpad_minidump_test_support mini_chromium crashpad_tool_support)

if(WINDOWS_MSVC_X86_64)
  android_add_executable(TARGET fastfail_program NODISTRIBUTE
                         SRC win/fastfail_test_program.cc)
  target_link_libraries(fastfail_program PRIVATE crashpad_client crashpad_util)

  android_add_executable(TARGET crashpad_handler_com NODISTRIBUTE SRC main.cc)
  target_link_libraries(
    crashpad_handler_com PRIVATE crashpad_handler_handler crashpad_util
                                 crashpad_tool_support)
  set_target_properties(crashpad_handler_com
                        PROPERTIES SUFFIX ".com" OUTPUT_NAME "crashpad_handler")

  android_add_executable(TARGET crash_other_program NODISTRIBUTE
                         SRC win/crash_other_program.cc)
  target_link_libraries(
    crash_other_program PRIVATE crashpad_client crashpad_util crashpad_test
                                gtest)

  android_add_executable(TARGET crashy_program NODISTRIBUTE
                         SRC win/crashy_test_program.cc)

  target_compile_definitions(crashy_program PRIVATE UNICODE)
  target_link_libraries(crashy_program PRIVATE crashpad_client)

  android_add_executable(TARGET self_destroying_program NODISTRIBUTE
                         SRC win/self_destroying_test_program.cc)

  target_compile_definitions(self_destroying_program PRIVATE UNICODE)
  target_link_libraries(self_destroying_program PRIVATE crashpad_client
                                                        crashpad_snapshot)

  android_add_executable(TARGET crashy_signal NODISTRIBUTE
                         SRC win/crashy_signal.cc)
  target_link_libraries(crashy_signal PRIVATE crashpad_client crashpad_util)

  android_add_executable(
    TARGET fake_handler_that_crashes_at_startup NODISTRIBUTE
    SRC win/fake_handler_that_crashes_at_startup.cc)

  android_add_executable(TARGET hanging_program NODISTRIBUTE
                         SRC win/hanging_program.cc)
  target_link_libraries(hanging_program PRIVATE crashpad_client crashpad_util)
  target_compile_definitions(hanging_program PRIVATE UNICODE)
endif()
