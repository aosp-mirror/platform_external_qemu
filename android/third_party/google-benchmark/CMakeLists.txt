cmake_minimum_required(VERSION 3.5)

set(src "${ANDROID_QEMU2_TOP_DIR}/../google-benchmark/src")

android_add_library(
  TARGET emulator-gbench
  NODISTRIBUTE
  SRC # cmake-format: sortable
      ${src}/benchmark.cc
      ${src}/colorprint.cc
      ${src}/commandlineflags.cc
      ${src}/complexity.cc
      ${src}/console_reporter.cc
      ${src}/csv_reporter.cc
      ${src}/json_reporter.cc
      ${src}/log.cc
      ${src}/re_posix.cc
      ${src}/reporter.cc
      ${src}/sleep.cc
      ${src}/string_util.cc
      ${src}/sysinfo.cc
      ${src}/walltime.cc)

target_include_directories(emulator-gbench PUBLIC ${src}/../include)
# NOTE HAVE_STD_REGEX doesn't seem to work with our toolchains.
target_compile_definitions(emulator-gbench PRIVATE -DHAVE_POSIX_REGEX=1)
target_compile_options(emulator-gbench PRIVATE -Wno-missing-braces)

android_target_link_libraries(emulator-gbench linux-x86_64 PUBLIC pthread)
android_target_link_libraries(emulator-gbench darwin-x86_64 PUBLIC pthread)
android_target_link_libraries(emulator-gbench windows
                              PUBLIC emulator-regex-win32 shlwapi::shlwapi)
