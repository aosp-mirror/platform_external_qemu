cmake_minimum_required(VERSION 3.5)

if (WINDOWS)
    # Create an empty dummy lib..
    add_library(emulator-libselinux ${ANDROID_QEMU2_TOP_DIR}/dummy.c)
else ()
    add_library(emulator-libselinux
            src/callbacks.c
            src/check_context.c
            src/freecon.c
            src/init.c
            src/label.c
            src/label_file.c
            src/label_android_property.c
            )
    target_link_libraries(emulator-libselinux PRIVATE android-emu-base)
    target_include_directories(emulator-libselinux PUBLIC include)
    target_include_directories(emulator-libselinux PRIVATE .)
    if (DARWIN_X86_64)
        target_compile_definitions(emulator-libselinux PRIVATE -DDARWIN)
    endif ()
    target_compile_definitions(emulator-libselinux PRIVATE -DHOST)
endif ()
android_license(emulator-libselinux "android/third_party/libselinux/NOTICE")