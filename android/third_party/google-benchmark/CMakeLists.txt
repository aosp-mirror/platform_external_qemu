cmake_minimum_required(VERSION 3.5)

set(src "${ANDROID_QEMU2_TOP_DIR}/../google-benchmark/src")

set(emulator-gbench_src
    ${src}/benchmark.cc
    ${src}/colorprint.cc
    ${src}/commandlineflags.cc
    ${src}/complexity.cc
    ${src}/console_reporter.cc
    ${src}/csv_reporter.cc
    ${src}/json_reporter.cc
    ${src}/log.cc
    ${src}/reporter.cc
    ${src}/re_posix.cc
    ${src}/sleep.cc
    ${src}/string_util.cc
    ${src}/sysinfo.cc
    ${src}/walltime.cc)

set(emulator-gbench_includes_public ${src}/../include)
# NOTE HAVE_STD_REGEX doesn't seem to work with our toolchains.
set(emulator-gbench_defs_private -DHAVE_POSIX_REGEX=1)

set(emulator-gbench_linux-x86_64_libs_public pthread)
set(emulator-gbench_darwin-x86_64_libs_public pthread)
set(emulator-gbench_windows-x86_64_includes_public compat)
set(emulator-gbench_windows-x86_64_libs_public emulator-regex-win32 -lshlwapi)

set(emulator-gbench_windows-x86_includes_public compat)
set(emulator-gbench_windows-x86_libs_public emulator-regex-win32 -lshlwapi)
add_android_library(emulator-gbench)
