android_add_library(
  TARGET crashpad_test
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      errors.cc
      file.cc
      filesystem.cc
      hex_string.cc
      main_arguments.cc
      multiprocess_exec.cc
      process_type.cc
      scoped_module_handle.cc
      scoped_temp_dir.cc
      test_paths.cc
  LINUX # cmake-format: sortable
        linux/fake_ptrace_connection.cc linux/get_tls.cc
        multiprocess_exec_posix.cc multiprocess_posix.cc
        scoped_guarded_page_posix.cc scoped_temp_dir_posix.cc
  DARWIN # cmake-format: sortable
         mac/dyld.cc
         mac/exception_swallower.cc
         mac/mach_errors.cc
         mac/mach_multiprocess.cc
         multiprocess_exec_posix.cc
         multiprocess_posix.cc
         scoped_guarded_page_posix.cc
         scoped_temp_dir_posix.cc
  WINDOWS # cmake-format: sortable
          multiprocess_exec_win.cc
          scoped_guarded_page_win.cc
          scoped_temp_dir_win.cc
          win/child_launcher.cc
          win/win_child_process.cc
          win/win_multiprocess.cc
          win/win_multiprocess_with_temp_dir.cc)

target_link_libraries(crashpad_test PUBLIC crashpad_util crashpad_handler_handler
                                           mini_chromium gtest)
android_target_link_libraries(crashpad_test darwin-aarch64
                              PRIVATE "-framework Security" bsm)
android_target_link_libraries(crashpad_test darwin-x86_64
                              PRIVATE "-framework Security" bsm)

android_add_library(TARGET crashpad_googlemock_main NODISTRIBUTE SRC gtest_main.cc)
target_link_libraries(crashpad_googlemock_main PRIVATE gmock crashpad_util crashpad_test)
target_compile_definitions(crashpad_googlemock_main PUBLIC CRASHPAD_TEST_LAUNCHER_GOOGLEMOCK)

android_add_test(
  TARGET crashpad_test_test
  SRC # cmake-format: sortable
      hex_string_test.cc main_arguments_test.cc multiprocess_exec_test.cc
      scoped_guarded_page_test.cc scoped_temp_dir_test.cc test_paths_test.cc
  LINUX # cmake-format: sortable
        multiprocess_posix_test.cc
  DARWIN # cmake-format: sortable
         mac/mach_multiprocess_test.cc multiprocess_posix_test.cc
  WINDOWS # cmake-format: sortable
          win/win_child_process_test.cc win/win_multiprocess_test.cc)
      target_link_libraries(crashpad_test_test PRIVATE crashpad_test gmock crashpad_googlemock_main Threads::Threads)

set_property(
  TEST crashpad_test_test APPEND
  PROPERTY ENVIRONMENT "CRASHPAD_TEST_DATA_ROOT=${CRASHPAD_TEST_DATA_ROOT}")

android_add_executable(TARGET crashpad_test_test_multiprocess_exec_test_child
                       NODISTRIBUTE SRC multiprocess_exec_test_child.cc)

target_link_libraries(crashpad_test_test_multiprocess_exec_test_child
                      PUBLIC crashpad_util)
add_dependencies(crashpad_test_test
                 crashpad_test_test_multiprocess_exec_test_child)
