cmake_minimum_required(VERSION 3.5)

# COMPILER + PATH CONFIGURATION
# =============================
# TODO(jansene): This section is needed as a bridge
# to the android build system.

# Use same compiler config as android build system
set(CMAKE_C_FLAGS ${LOCAL_CFLAGS})
set(CMAKE_CXX_FLAGS  "${LOCAL_CFLAGS} ${LOCAL_CXXFLAGS}")

# Work around some gcc/mingw issues
if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()


# Make sure we create archives in the same way..
SET(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")
SET(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")

# END COMPILER CONFIGURATION
# ==========================
if(WIN32 OR ("${LOCAL_OS}" STREQUAL "windows") )
    set(PLATFORM_SELINUX_SOURCES "")
else()
    set(PLATFORM_SELINUX_SOURCES
        src/callbacks.c
        src/check_context.c
        src/freecon.c
        src/init.c
        src/label.c
        src/label_file.c
        src/label_android_property.c
        )
endif()

add_library(emulator-libselinux ${PLATFORM_SELINUX_SOURCES})

target_include_directories(emulator-libselinux PUBLIC include)
target_include_directories(emulator-libselinux PRIVATE .)
if (APPLE)
    target_compile_definitions(emulator-libselinux PRIVATE -DDARWIN)
endif()
target_compile_definitions(emulator-libselinux PUBLIC -DHOST)
