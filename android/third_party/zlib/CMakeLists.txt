set(ZLIB_SOURCE_DIR ${ANDROID_QEMU2_TOP_DIR}/../zlib)
# ============================================================================
# zlib
# ============================================================================
# Note we have disabled the assembly options.

# parse the full version number from zlib.h and include in ZLIB_FULL_VERSION
file(READ ${ZLIB_SOURCE_DIR}/zlib.h _zlib_h_contents)
string(REGEX REPLACE ".*#define[ \t]+ZLIB_VERSION[ \t]+\"([-0-9A-Za-z.]+)\".*"
                     "\\1" ZLIB_FULL_VERSION ${_zlib_h_contents})

message(STATUS "Including zlib version: ${ZLIB_FULL_VERSION}")
android_add_library(
  TARGET zlib
  LICENSE NO_COPYRIGHT
  SRC # cmake-format: sortable
      ${ZLIB_SOURCE_DIR}/adler32.c
      ${ZLIB_SOURCE_DIR}/compress.c
      ${ZLIB_SOURCE_DIR}/crc32.c
      ${ZLIB_SOURCE_DIR}/deflate.c
      ${ZLIB_SOURCE_DIR}/gzclose.c
      ${ZLIB_SOURCE_DIR}/gzlib.c
      ${ZLIB_SOURCE_DIR}/gzread.c
      ${ZLIB_SOURCE_DIR}/gzwrite.c
      ${ZLIB_SOURCE_DIR}/infback.c
      ${ZLIB_SOURCE_DIR}/inffast.c
      ${ZLIB_SOURCE_DIR}/inflate.c
      ${ZLIB_SOURCE_DIR}/inftrees.c
      ${ZLIB_SOURCE_DIR}/trees.c
      ${ZLIB_SOURCE_DIR}/uncompr.c
      ${ZLIB_SOURCE_DIR}/zutil.c)
target_include_directories(zlib PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
                                       ${ZLIB_SOURCE_DIR})

android_target_compile_definitions(zlib linux-x86_64 PRIVATE
                                   -D_LARGEFILE64_SOURCE=1)
android_target_compile_definitions(zlib darwin-x86_64 PRIVATE
                                   -D_LARGEFILE64_SOURCE=1)
android_target_compile_definitions(
  zlib
  windows_msvc-x86_64
  PRIVATE
  -DWIN32
  -D_WIN32
  -D_CRT_NONSTDC_NO_DEPRECATE
  -D_CRT_SECURE_NO_DEPRECATE
  -D_CRT_NONSTDC_NO_WARNINGS
  -DZLIB_WINAPI
  -DNDEBUG
  -DWIN64)
