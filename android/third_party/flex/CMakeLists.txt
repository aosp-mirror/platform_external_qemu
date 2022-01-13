# We might be running in cross compile build, so setup the required paths and includes..
cmake_minimum_required(VERSION 3.5)
project(flex)

if(NOT ANDROID_QEMU2_TOP_DIR)
  get_filename_component(ANDROID_QEMU2_TOP_DIR
                         "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)
  get_filename_component(
    ADD_PATH "${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/" ABSOLUTE)
  list(APPEND CMAKE_MODULE_PATH "${ADD_PATH}")
endif()

include(android)
include(prebuilts)

set(FLEX_ROOT ${ANDROID_QEMU2_TOP_DIR}/../flex/src)

android_yacc_compile(EXT c GENERATED flex_lib_yacc_src SRC ${FLEX_ROOT}/parse.y
                     SOURCE_DIR ${FLEX_ROOT})

android_add_executable(
  TARGET
    flex
    URL
    "https://android.googlesource.com/platform/external/bison/+/refs/heads/master"
  REPO "${FLEX_ROOT}"
  NOTICE "REPO/NOTICE"
  LICENSE "GPL3"
  SRC ${flex_lib_yacc_src}
      ${FLEX_ROOT}/buf.c
      ${FLEX_ROOT}/ccl.c
      ${FLEX_ROOT}/dfa.c
      ${FLEX_ROOT}/ecs.c
      ${FLEX_ROOT}/filter.c
      ${FLEX_ROOT}/gen.c
      ${FLEX_ROOT}/main.c
      ${FLEX_ROOT}/misc.c
      ${FLEX_ROOT}/nfa.c
      ${FLEX_ROOT}/options.c
      ${FLEX_ROOT}/regex.c
      ${FLEX_ROOT}/scan.l
      ${FLEX_ROOT}/scanflags.c
      ${FLEX_ROOT}/scanopt.c
      ${FLEX_ROOT}/sym.c
      ${FLEX_ROOT}/tables.c
      ${FLEX_ROOT}/tables_shared.c
      ${FLEX_ROOT}/tblcmp.c
      ${FLEX_ROOT}/yylex.c
      # Fun fact, you cannot compile flex without having flex.. So we just
      # include the generated flex scan.l --> scan.c
      flex_skel.c
      scan.c)

android_target_compile_definitions(
  flex
  darwin-x86_64
  PRIVATE -DHAVE_ALLOCA=1
          -DHAVE_LIMITS_H=1
          -DHAVE_UNISTD_H=1
          -DHAVE_NETINET_IN_H=1
          -DHAVE_SYS_WAIT_H=1
          -DHAVE_STDBOOL_H=1
          -DHAVE_REGEX_H=1
          -DSTDC_HEADERS=1
          -DVERSION="2.6.4"
          # TODO: This should use a hermetic m4 -- this is a default that can be
          # overwritten overwritten with the M4 environment variable, which may
          # be simpler within the build.
          -DM4="m4")

android_target_compile_definitions(
  flex
  linux-x86_64
  PRIVATE -DHAVE_ALLOCA=1
          -DHAVE_LIMITS_H=1
          -DHAVE_UNISTD_H=1
          -DHAVE_NETINET_IN_H=1
          -DHAVE_SYS_WAIT_H=1
          -DHAVE_STDBOOL_H=1
          -DHAVE_REGEX_H=1
          -DSTDC_HEADERS=1
          -DVERSION="2.6.4"
          # TODO: This should use a hermetic m4 -- this is a default that can be
          # overwritten overwritten with the M4 environment variable, which may
          # be simpler within the build.
          -DM4="m4")


android_target_compile_definitions(
  flex
  windows
  PRIVATE -DHAVE_ALLOCA=1
          -DHAVE_LIMITS_H=1
          -DHAVE_STDBOOL_H=1
          -DHAVE_REGEX_H=1
          -DSTDC_HEADERS=1
          -DVERSION="2.6.4"
          # TODO: This should use a hermetic m4 -- this is a default that can be
          # overwritten overwritten with the M4 environment variable, which may
          # be simpler within the build.
          -DM4="m4")
target_compile_options(flex PRIVATE -std=c99 -Wno-error)
target_link_libraries(flex PRIVATE m)

android_target_include_directories(flex windows PRIVATE msvc)
target_include_directories(flex PRIVATE ${FLEX_ROOT}
                                        ${CMAKE_CURRENT_BINARY_DIR}/yacc)
android_target_link_libraries(flex windows PRIVATE emulator-regex-win32)
