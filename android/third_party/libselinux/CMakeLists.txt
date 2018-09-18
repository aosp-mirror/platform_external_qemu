cmake_minimum_required(VERSION 3.5)

if (WIN32 OR ("${LOCAL_OS}" STREQUAL "windows"))
    # Create an empty dummy lib..
    add_library(emulator-libselinux src/label_internal.h)
    set_property(TARGET emulator-libselinux PROPERTY LINKER_LANGUAGE CXX)
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
    target_include_directories(emulator-libselinux PUBLIC include)
    target_include_directories(emulator-libselinux PRIVATE .)
    if (APPLE)
        target_compile_definitions(emulator-libselinux PRIVATE -DDARWIN)
    endif ()
    target_compile_definitions(emulator-libselinux PUBLIC -DHOST)
endif ()

