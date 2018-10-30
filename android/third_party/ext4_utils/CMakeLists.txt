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
    src/wipe.c)
android_add_library(emulator-libext4_utils)

target_include_directories(emulator-libext4_utils PUBLIC include)
target_link_libraries(emulator-libext4_utils PRIVATE
                              ZLIB::ZLIB
                              android-emu-base)
target_link_libraries(emulator-libext4_utils 
                              PUBLIC
                              emulator-libsparse
                              emulator-libselinux)
target_compile_options(emulator-libext4_utils PRIVATE -Wno-error)
target_compile_definitions(emulator-libext4_utils PRIVATE -DHOST)

# Windows specific settings
android_target_link_libraries(emulator-libext4_utils windows PUBLIC emulator-libmman-win32 -lws2_32)
android_target_compile_definitions(emulator-libext4_utils windows PRIVATE -DUSE_MINGW=1)

set(emulator_make_ext4fs_src src/make_ext4fs_main.c)
android_add_executable(emulator_make_ext4fs)
target_link_libraries(emulator_make_ext4fs PRIVATE emulator-libext4_utils)
target_compile_definitions(emulator_make_ext4fs PRIVATE -DHOST)

# Windows specific settings
android_target_link_libraries(emulator_make_ext4fs windows PUBLIC emulator-libmman-win32)
android_target_link_libraries(emulator_make_ext4fs windows_msvc PUBLIC msvc-posix-compat)
android_target_compile_definitions(emulator_make_ext4fs windows PRIVATE -DUSE_MINGW=1)
