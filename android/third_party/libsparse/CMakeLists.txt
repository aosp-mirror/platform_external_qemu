cmake_minimum_required(VERSION 3.5)

set(emulator-libsparse_src)
android_add_library(
  TARGET emulator-libsparse
  LIBNAME libsparse
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      src/backed_block.c src/output_file.c src/sparse.c src/sparse_crc32.c
      src/sparse_err.c src/sparse_read.c)

target_include_directories(emulator-libsparse PUBLIC include)
target_include_directories(emulator-libsparse PRIVATE src)
target_link_libraries(emulator-libsparse PRIVATE zlib)
android_target_link_libraries(emulator-libsparse windows
                              PRIVATE emulator-libmman-win32)
target_compile_options(emulator-libsparse PRIVATE -Wno-error)

android_add_executable(
  TARGET emulator_img2simg LICENSE Apache-2.0 SRC # cmake-format: sortable
                                                  src/img2simg.c)
target_link_libraries(emulator_img2simg PRIVATE emulator-libsparse)

android_add_executable(
  TARGET emulator_simg2img LICENSE Apache-2.0 SRC # cmake-format: sortable
                                                  src/simg2img.c)
target_link_libraries(emulator_simg2img PRIVATE emulator-libsparse)
