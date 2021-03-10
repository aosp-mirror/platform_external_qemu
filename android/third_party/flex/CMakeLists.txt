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
  LICENSE "GPL3" SOURCE_DIR ${FLEX_ROOT}
  SRC ${CMAKE_CURRENT_SOURCE_DIR}/flex_skel.c
      ${CMAKE_CURRENT_SOURCE_DIR}/scan.c
      ${flex_lib_yacc_src}
      buf.c
      ccl.c
      dfa.c
      ecs.c
      filter.c
      gen.c
      main.c
      misc.c
      nfa.c
      options.c
      regex.c
      scan.l
      scanflags.c
      scanopt.c
      sym.c
      tables.c
      tables_shared.c
      tblcmp.c
      yylex.c
      # Fun fact, you cannot compile flex without having flex.. So we just
      # include the generated flex scan.l --> scan.c
)

target_compile_definitions(
  flex
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

target_compile_options(flex PRIVATE -std=c99 -Wno-pointer-sign)
target_link_libraries(flex PRIVATE m)
target_include_directories(flex PRIVATE ${FLEX_ROOT}
                                        ${CMAKE_CURRENT_BINARY_DIR}/yacc)
