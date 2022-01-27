android_add_library(
  TARGET mini_chromium
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      debug/alias.cc
      files/file_path.cc
      files/scoped_file.cc
      logging.cc
      process/memory.cc
      rand_util.cc
      strings/string_number_conversions.cc
      strings/stringprintf.cc
      strings/utf_string_conversion_utils.cc
      strings/utf_string_conversions.cc
      synchronization/lock.cc
      third_party/icu/icu_utf.cc
      threading/thread_local_storage.cc
  LINUX # cmake-format: sortable
        files/file_util_posix.cc
        memory/page_size_posix.cc
        posix/safe_strerror.cc
        synchronization/condition_variable_posix.cc
        synchronization/lock_impl_posix.cc
        threading/thread_local_storage_posix.cc
  DARWIN # cmake-format: sortable
         files/file_util_posix.cc
         mac/close_nocancel.cc
         mac/foundation_util.mm
         mac/mach_logging.cc
         mac/scoped_mach_port.cc
         mac/scoped_mach_vm.cc
         mac/scoped_nsautorelease_pool.mm
         memory/page_size_posix.cc
         posix/safe_strerror.cc
         strings/sys_string_conversions_mac.mm
         synchronization/condition_variable_posix.cc
         synchronization/lock_impl_posix.cc
         threading/thread_local_storage_posix.cc
  WINDOWS # cmake-format: sortable
          memory/page_size_win.cc scoped_clear_last_error_win.cc
          strings/string_util_win.cc synchronization/lock_impl_win.cc
          threading/thread_local_storage_win.cc)

android_target_compile_definitions(mini_chromium windows PUBLIC UNICODE)
android_target_link_libraries(
  mini_chromium darwin-x86_64
  PUBLIC "-framework ApplicationServices" "-framework CoreFoundation"
         "-framework Foundation" "-framework IOKit" "-framework Security")

android_target_link_libraries(
    mini_chromium  darwin-aarch64
  PUBLIC "-framework ApplicationServices" "-framework CoreFoundation"
         "-framework Foundation" "-framework IOKit" "-framework Security")
target_link_libraries(mini_chromium PUBLIC Threads::Threads)

android_target_link_libraries(mini_chromium  windows PUBLIC advapi32::advapi32)
target_include_directories(mini_chromium PUBLIC ..)
