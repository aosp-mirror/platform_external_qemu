prebuilt(ZLIB)
set(emulator-libext4_utils_src
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

set(emulator-libext4_utils_includes_public include)
set(emulator-libext4_utils_libs_public
    emulator-libsparse
    emulator-libselinux
    ${ZLIB_LIBRARIES}
    android-emu-base)
set(emulator-libext4_utils_libs_windows-x86_64_public  emulator-libmman-win32)
set(emulator-libext4_utils_libs_windows-x86_public  emulator-libmman-win32)
set(emulator-libext4_utils_defs_private -DHOST)
set(emulator-libext4_utils_windows-x86_64_defs_private -DUSE_MINGW=1)
set(emulator-libext4_utils_windows-x86_defs_private -DUSE_MINGW=1)
set(emulator-libext4_utils_compile_options_private -Wno-error)

add_android_library(emulator-libext4_utils)

set(emulator_make_ext4fs_src src/make_ext4fs_main.c)
set(emulator_make_ext4fs_libs_private emulator-libext4_utils)
set(emulator_make_ext4fs_defs_private -DHOST)

add_android_executable(emulator_make_ext4fs)

