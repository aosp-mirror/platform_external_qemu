set(ZLIB_SOURCE_DIR ${ANDROID_QEMU2_TOP_DIR}/../zlib)

set(ZLIB_PC ${CMAKE_CURRENT_BINARY_DIR}/zlib.pc)
configure_file(${ZLIB_SOURCE_DIR}/zlib.pc.cmakein ${ZLIB_PC} @ONLY)
configure_file(${ZLIB_SOURCE_DIR}/zconf.h.cmakein ${CMAKE_CURRENT_BINARY_DIR}/zconf.h @ONLY)

# ============================================================================
# zlib
# ============================================================================

set(ZLIB_ASMS ${ZLIB_SOURCE_DIR}/contrib/amd64/amd64-match.S)
set(zlib_src
    ${ZLIB_SOURCE_DIR}/adler32.c
    ${ZLIB_SOURCE_DIR}/compress.c
    ${ZLIB_SOURCE_DIR}/crc32.c
    ${ZLIB_SOURCE_DIR}/deflate.c
    ${ZLIB_SOURCE_DIR}/gzclose.c
    ${ZLIB_SOURCE_DIR}/gzlib.c
    ${ZLIB_SOURCE_DIR}/gzread.c
    ${ZLIB_SOURCE_DIR}/gzwrite.c
    ${ZLIB_SOURCE_DIR}/inflate.c
    ${ZLIB_SOURCE_DIR}/infback.c
    ${ZLIB_SOURCE_DIR}/inftrees.c
    ${ZLIB_SOURCE_DIR}/inffast.c
    ${ZLIB_SOURCE_DIR}/trees.c
    ${ZLIB_SOURCE_DIR}/uncompr.c
    ${ZLIB_SOURCE_DIR}/zutil.c
    ${ZLIB_ASMS})
set_source_files_properties(${ZLIB_ASMS} PROPERTIES LANGUAGE C)

# parse the full version number from zlib.h and include in ZLIB_FULL_VERSION
file(READ ${ZLIB_SOURCE_DIR}/zlib.h _zlib_h_contents)
string(REGEX
       REPLACE ".*#define[ \t]+ZLIB_VERSION[ \t]+\"([-0-9A-Za-z.]+)\".*" "\\1" ZLIB_FULL_VERSION ${_zlib_h_contents})

android_add_library(zlib)
target_include_directories(zlib PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${ZLIB_SOURCE_DIR})

target_compile_definitions(zlib PRIVATE -D_LARGEFILE64_SOURCE=1)
target_compile_definitions(zlib PRIVATE -DASMV)

android_target_compile_definitions(zlib
                                   windows
                                   PRIVATE
                                   -D_CRT_SECURE_NO_DEPRECATE
                                   -D_CRT_NONSTDC_NO_DEPRECATE
                                   -DNO_FSEEKO
                                   -D_WIN32 # Make sure we define widechar for unicode translation and usage of _wopen vs non unicode variant.
)
