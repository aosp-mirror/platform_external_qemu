android_add_library(
  TARGET android-emu-base-process # SHARED # Testing only
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/base/process/Command.cpp
  LINUX android/base/process/Command_Posix.cpp
  DARWIN android/base/process/Command_Posix.cpp
  WINDOWS android/base/process/Command_Windows.cpp
  DEPS android-emu-base android-emu-base-headers android-emu-base-logging
       android-emu-base-streams android-emu-utils)

target_include_directories(android-emu-base-process PRIVATE src)
target_link_libraries(android-emu-base-process PRIVATE android-emu-base-headers)

# We cannot run these tests, and they introduce additional dependencies that
# require cross compiling as well, so we ignore these for now.
if(NOT CROSSCOMPILING AND NOT NETSIM_EXT)
  android_add_test(
    TARGET android-emu-base-process_unittests
    SOURCE_DIR test
    SRC android/base/process/Command_unittest.cpp
    DEPS android-emu-base android-emu-base-headers android-emu-base-process
         android-emu-base-streams android-emu-files gmock_main)
  target_include_directories(android-emu-base-process_unittests PRIVATE src)
  target_compile_definitions(android-emu-base-process_unittests
                             PRIVATE _GLIBCXX_USE_NANOSLEEP=1)

  android_add_executable(
    TARGET sleep_emu LICENESE NODISTRIBUTE SOURCE_DIR test
    SRC android/base/process/sleep.cpp DEPS absl::flags absl::flags_parse
                                            absl::time)

  add_dependencies(android-emu-base-process_unittests sleep_emu)
endif()
