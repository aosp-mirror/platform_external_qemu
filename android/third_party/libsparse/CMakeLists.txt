cmake_minimum_required(VERSION 3.5)

set(emulator-libsparse_src
        src/backed_block.c
        src/output_file.c
        src/sparse.c
        src/sparse_crc32.c
        src/sparse_err.c
        src/sparse_read.c)
android_add_library(emulator-libsparse)

target_include_directories(emulator-libsparse PUBLIC include)
target_include_directories(emulator-libsparse PRIVATE src)
target_link_libraries(emulator-libsparse PRIVATE zlib)
android_target_link_libraries(emulator-libsparse windows PRIVATE emulator-libmman-win32)

if (WINDOWS)
    # Work around some gcc/mingw issues
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_definitions(emulator-libsparse PRIVATE -DUSE_MINGW=1)
    endif ()
endif ()

target_compile_options(emulator-libsparse PRIVATE -Wno-error)

set(emulator_img2simg_src src/img2simg.c)
android_add_executable(emulator_img2simg)
target_link_libraries(emulator_img2simg PRIVATE emulator-libsparse)

set(emulator_simg2img_src src/simg2img.c)
android_add_executable(emulator_simg2img)
target_link_libraries(emulator_simg2img PRIVATE emulator-libsparse)