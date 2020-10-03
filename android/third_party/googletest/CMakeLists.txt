# Let's not install gtest..
# set(INSTALL_GTEST OFF CACHE BOOL "Never install gtest" FORCE)
set(GOOGLETEST_VERSION 1.10.0)
set(gtest_SOURCE_DIR ${ANDROID_QEMU2_TOP_DIR}/../googletest/googletest)
set(gmock_SOURCE_DIR ${ANDROID_QEMU2_TOP_DIR}/../googletest/googlemock)

set(gtest_build_include_dirs "${gtest_SOURCE_DIR}/include"
                             "${gtest_SOURCE_DIR}")

# Adds Google Mock's and Google Test's header directories to the search path.
set(gmock_build_include_dirs
    "${gmock_SOURCE_DIR}/include"
    "${gmock_SOURCE_DIR}"
    "${gtest_SOURCE_DIR}/include"
    # This directory is needed to build directly from Google Test sources.
    "${gtest_SOURCE_DIR}")

android_add_library(
  TARGET gtest
  LICENSE "BSD3"
  LIBNAME
    gtest
    URL
    "https://android.googlesource.com/platform/external/googletest/+/refs/heads/master"
  REPO "${ANDROID_QEMU2_TOP_DIR}"
  NOTICE "REPO/LICENSE"
  SRC # cmake-format: sortable
      ${gtest_SOURCE_DIR}/src/gtest-all.cc)

set_target_properties(gtest PROPERTIES VERSION ${GOOGLETEST_VERSION})
target_include_directories(gtest PUBLIC ${gtest_build_include_dirs})

android_add_library(
  TARGET gtest_main
  LICENSE "BSD3"
  LIBNAME
    gtest
    URL
    "https://android.googlesource.com/platform/external/googletest/+/refs/heads/master"
  REPO "${ANDROID_QEMU2_TOP_DIR}"
  NOTICE "REPO/LICENSE"
  SRC # cmake-format: sortable
      ${gtest_SOURCE_DIR}/src/gtest_main.cc)

set_target_properties(gtest_main PROPERTIES VERSION ${GOOGLETEST_VERSION})
target_include_directories(gtest_main PUBLIC ${gtest_build_include_dirs})
target_link_libraries(gtest_main PRIVATE gtest PUBLIC Threads::Threads)

android_add_library(
  TARGET gmock
  LICENSE "BSD3"
  LIBNAME
    gtest
    URL
    "https://android.googlesource.com/platform/external/googletest/+/refs/heads/master"
  REPO "${ANDROID_QEMU2_TOP_DIR}"
  NOTICE "REPO/LICENSE"
  SRC # cmake-format: sortable
      "${gmock_SOURCE_DIR}/src/gmock-all.cc")
target_include_directories(gmock PUBLIC ${gmock_build_include_dirs})
target_link_libraries(gmock PRIVATE gtest)
target_compile_options(gmock PUBLIC -Wno-inconsistent-missing-override)

android_add_library(
  TARGET gmock_main
  LICENSE "BSD3"
  LIBNAME
    gtest
    URL
    "https://android.googlesource.com/platform/external/googletest/+/refs/heads/master"
  REPO "${ANDROID_QEMU2_TOP_DIR}"
  NOTICE "REPO/LICENSE"
  SRC # cmake-format: sortable
      "${gmock_SOURCE_DIR}/src/gmock_main.cc")

target_include_directories(gmock_main PUBLIC ${gmock_build_include_dirs})
target_link_libraries(gmock_main PRIVATE gmock PUBLIC Threads::Threads)

if(WINDOWS_MSVC_X86_64)
  # Okay, google test is not happy under msvc compile
  foreach(tgt gtest gmock gmock_main gtest_main)
    # clang-cl cross compile does not like the msvc specific flags.
    if(MSVC)
      target_compile_options(${tgt} PRIVATE -EHsc)
      target_compile_definitions(${tgt} PRIVATE -D_HAS_EXCEPTIONS=1)
    endif()
    target_compile_definitions(
      ${tgt} PUBLIC -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
  endforeach()
else()
  # Enable type information and exceptions in the test framework, and compile it
  # with full debug info
  foreach(tgt gtest gmock gmock_main gtest_main)
    target_compile_options(${tgt} PRIVATE -frtti -O0 -fexceptions)
  endforeach()
endif()

if(WINDOWS_MSVC_X86_64)
  # Don't include the custom TempDir function. It is for Android only.
  target_compile_definitions(
    gtest PRIVATE -DGTEST_INCLUDE_GTEST_INTERNAL_CUSTOM_GTEST_H_)
endif()
