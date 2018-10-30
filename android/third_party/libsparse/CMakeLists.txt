cmake_minimum_required(VERSION 3.5)

prebuilt(ZLIB)

add_library(emulator-libsparse
        src/backed_block.c
        src/output_file.c
        src/sparse.c
        src/sparse_crc32.c
        src/sparse_err.c
        src/sparse_read.c
        )

target_include_directories(emulator-libsparse PUBLIC include)
target_include_directories(emulator-libsparse PRIVATE src)
target_link_libraries(emulator-libsparse PRIVATE ZLIB::ZLIB)
android_target_link_libraries(emulator-libsparse windows PRIVATE emulator-libmman-win32)
android_target_link_libraries(emulator-libsparse windows_msvc PRIVATE msvc-posix-compat)

if (ANDROID_TARGET_TAG MATCHES "windows.*")
    # Work around some gcc/mingw issues
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_definitions(emulator-libsparse PRIVATE -DUSE_MINGW=1)
    endif ()
endif ()

target_compile_options(emulator-libsparse PRIVATE -Wno-error)

add_executable(emulator_img2simg src/img2simg.c)
target_link_libraries(emulator_img2simg PRIVATE emulator-libsparse)
android_target_link_libraries(emulator_img2simg windows PRIVATE emulator-libmman-win32)
android_target_link_libraries(emulator_img2simg windows_msvc PRIVATE msvc-posix-compat)

add_executable(emulator_simg2img src/simg2img.c)
target_link_libraries(emulator_simg2img PRIVATE emulator-libsparse)
android_target_link_libraries(emulator_simg2img windows PRIVATE emulator-libmman-win32)
android_target_link_libraries(emulator_simg2img windows_msvc PRIVATE msvc-posix-compat)