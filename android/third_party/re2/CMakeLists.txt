# Copyright 2015 The RE2 Authors.  All Rights Reserved.
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file.

option(RE2_USE_ICU "build against ICU for full Unicode properties support" OFF)

# For historical reasons, this is just "USEPCRE", not "RE2_USE_PCRE".
option(USEPCRE "build against PCRE for testing and benchmarking" OFF)

# See https://groups.google.com/g/re2-dev/c/P6_NM0YIWvA for details.
# This has no effect unless RE2 is being built for an Apple platform
# such as macOS or iOS.
option(RE2_BUILD_FRAMEWORK "build RE2 as a framework" OFF)

set(EXTRA_TARGET_LINK_LIBRARIES)

android_add_library(
  TARGET re2
  LIBNAME
    re2
  URL
    "https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev/android/third_party/re2/"
  REPO "${ANDROID_QEMU2_TOP_DIR}"
  LICENSE
    "https://android.googlesource.com/platform/external/qemu/+/refs/heads/emu-master-dev/android/third_party/re2/LICENSE"
  LICENSE "BSD-3-Clause"
  SRC re2/bitmap256.cc
      re2/bitstate.cc
      re2/compile.cc
      re2/dfa.cc
      re2/filtered_re2.cc
      re2/mimics_pcre.cc
      re2/nfa.cc
      re2/onepass.cc
      re2/parse.cc
      re2/perl_groups.cc
      re2/prefilter.cc
      re2/prefilter_tree.cc
      re2/prog.cc
      re2/re2.cc
      re2/regexp.cc
      re2/set.cc
      re2/simplify.cc
      re2/stringpiece.cc
      re2/tostring.cc
      re2/unicode_casefold.cc
      re2/unicode_groups.cc
      util/rune.cc
      util/strutil.cc)
target_include_directories(re2 PUBLIC .)
add_library(re2::re2 ALIAS re2)

if(WINDOWS_MSVC_X86_64)
  target_compile_definitions(
    re2 PRIVATE -DUNICODE -D_UNICODE -DSTRICT -DNOMINMAX
                -D_CRT_SECURE_NO_WARNINGS -D_SCL_SECURE_NO_WARNINGS)

  if(NOT CROSSCOMPILE)
    # CMake defaults to /W3, but some users like /W4 (or /Wall) and /WX, so we
    # disable various warnings that aren't particularly helpful.
    target_compile_options(
      re2
      PRIVATE /wd4100
              /wd4201
              /wd4456
              /wd4457
              /wd4702
              /wd4815
              /utf-8)
  endif()
endif()

if(APPLE AND RE2_BUILD_FRAMEWORK)
  set_target_properties(re2 PROPERTIES
    FRAMEWORK TRUE
    FRAMEWORK_VERSION A
    MACOSX_FRAMEWORK_IDENTIFIER com.googlesource.code.re2)
endif()

if(UNIX)
  target_link_libraries(re2 PUBLIC Threads::Threads)
endif()

if(RE2_USE_ICU)
  target_link_libraries(re2 PUBLIC ICU::uc)
endif()

if(USEPCRE)
  add_definitions(-DUSEPCRE)
  list(APPEND EXTRA_TARGET_LINK_LIBRARIES pcre)
endif()

android_add_library(
  TARGET testing
  NODISTRIBUTE
  SRC re2/testing/backtrack.cc
      re2/testing/dump.cc
      re2/testing/exhaustive_tester.cc
      re2/testing/null_walker.cc
      re2/testing/regexp_generator.cc
      re2/testing/string_generator.cc
      re2/testing/tester.cc
      util/pcre.cc)
target_link_libraries(testing PUBLIC re2)

set(TEST_TARGETS
    charclass_test
    compile_test
    filtered_re2_test
    mimics_pcre_test
    parse_test
    possible_match_test
    re2_test
    re2_arg_test
    regexp_test
    required_prefix_test
    search_test
    set_test
    simplify_test
    string_generator_test
    dfa_test

    # These tests can take a looong time, so lets not run them.. (>180s)
    # exhaustive1_test exhaustive2_test exhaustive3_test exhaustive_test
    # random_test
)

set(BENCHMARK_TARGETS regexp_benchmark)

foreach(target ${TEST_TARGETS})
  android_add_test(TARGET ${target} SRC re2/testing/${target}.cc util/test.cc)
  target_link_libraries(${target} PRIVATE testing
                                          ${EXTRA_TARGET_LINK_LIBRARIES})
endforeach()
