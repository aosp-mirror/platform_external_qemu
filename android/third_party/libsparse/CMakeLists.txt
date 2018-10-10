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

if (WIN32 OR ("${LOCAL_OS}" STREQUAL "windows"))
    # Work around some gcc/mingw issues
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_definitions(emulator-libsparse PRIVATE -DUSE_MINGW=1)
    endif ()
    target_include_directories(emulator-libsparse PRIVATE ../mman-win32/includes)
    target_include_directories(emulator-libsparse PUBLIC ${ZLIB_INCLUDE_DIRS})
endif ()

target_compile_options(emulator-libsparse PRIVATE -Wno-error)

message("Using ${ZLIB_LIBRARIES}")

add_executable(emulator_img2simg src/img2simg.c)
target_link_libraries(emulator_img2simg PRIVATE emulator-libsparse)
target_link_libraries(emulator_img2simg PRIVATE ${ZLIB_LIBRARIES})
if (WIN32 OR ("${LOCAL_OS}" STREQUAL "windows"))
    target_link_libraries(emulator_img2simg PRIVATE emulator-libmman-win32)
endif ()

add_executable(emulator_simg2img src/simg2img.c)
target_link_libraries(emulator_simg2img PRIVATE emulator-libsparse)

if (WIN32 OR ("${LOCAL_OS}" STREQUAL "windows"))
    target_link_libraries(emulator_simg2img PRIVATE emulator-libmman-win32)
endif ()
target_link_libraries(emulator_simg2img PRIVATE ${ZLIB_LIBRARIES})
