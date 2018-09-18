cmake_minimum_required(VERSION 3.5)
add_library(emulator-libext4_utils
        src/allocate.c
        src/contents.c
        src/crc16.c
        src/ext4_sb.c
        src/ext4_utils.c
        src/extent.c
        src/fs_config.c
        src/indirect.c
        src/make_ext4fs.c
        src/sha1.c
        src/uuid.c
        src/wipe.c
        )

target_include_directories(emulator-libext4_utils PUBLIC include)
target_include_directories(emulator-libext4_utils PRIVATE ../libselinux/include)

target_include_directories(emulator-libext4_utils PRIVATE ../libsparse/include)
target_include_directories(emulator-libext4_utils PRIVATE ../../android-emu/)

if (WIN32 OR ("${LOCAL_OS}" STREQUAL "windows"))
    # Work around some gcc/mingw issues
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_definitions(emulator-libext4_utils PRIVATE -DUSE_MINGW=1)
    endif ()
endif ()
target_compile_options(emulator-libext4_utils PRIVATE -Wno-error)
target_compile_definitions(emulator-libext4_utils PRIVATE -DHOST)

