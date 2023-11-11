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
  INCLUDES "${ZLIB_SOURCE_DIR}"
  LICENSE NO_COPYRIGHT
  SOURCE_DIR ${ZLIB_SOURCE_DIR}
  SRC adler32.c
      compress.c
      crc32.c
      deflate.c
      gzclose.c
      gzlib.c
      gzread.c
      gzwrite.c
      infback.c
      inffast.c
      inflate.c
      inftrees.c
      trees.c
      uncompr.c
      zutil.c)

android_target_compile_definitions(zlib linux-x86_64
                                   PRIVATE -D_LARGEFILE64_SOURCE=1)
android_target_compile_definitions(zlib darwin-x86_64
                                   PRIVATE -D_LARGEFILE64_SOURCE=1)
target_compile_options(zlib PRIVATE -Wno-implicit-function-declaration
                                    -Wno-deprecated-non-prototype)

# Note that this is the only non-bazel target.
android_target_compile_definitions(
  zlib windows_msvc-x86_64
  PRIVATE -DWIN32
          -D_WIN32
          -D_CRT_NONSTDC_NO_DEPRECATE
          -D_CRT_SECURE_NO_DEPRECATE
          -D_CRT_NONSTDC_NO_WARNINGS
          -DZLIB_WINAPI
          -DNDEBUG
          -DWIN64)
