# This code doesn't build on Windows
if(CMAKE_SYSTEM_NAME MATCHES "(Linux|Darwin)")
    add_library(selinux STATIC
        src/callbacks.c
        src/check_context.c
        src/freecon.c
        src/init.c
        src/label.c
        src/label_file.c
        src/label_android_property.c
        )

    target_include_directories(selinux
        PUBLIC
        ./include
        PRIVATE
        ./
        )

    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(SELINUX_PLATFORM_OPT -DDARWIN)
    endif()

    target_compile_options(selinux
        PUBLIC
        ${SELINUX_PLATFORM_OPT}
        -DHOST
        )
endif()
