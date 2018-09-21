add_library(emulator-gbench
    src/benchmark.cc
    src/colorprint.cc
    src/commandlineflags.cc
    src/complexity.cc
    src/console_reporter.cc
    src/csv_reporter.cc
    src/json_reporter.cc
    src/log.cc
    src/reporter.cc
    src/re_posix.cc
    src/sleep.cc
    src/string_util.cc
    src/sysinfo.cc
    src/walltime.cc
    )

add_executable(google_benchmark_basic_test test/basic_test.cc)
target_link_libraries(google_benchmark_basic_test PRIVATE emulator-gbench)


