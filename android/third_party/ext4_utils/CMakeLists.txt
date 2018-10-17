cmake_minimum_required(VERSION 3.5)
# COMPILER + PATH CONFIGURATION
# =============================
# TODO(jansene): This section is needed as a bridge
# to the android build system.

# Please do not try to test compile an executable..
# We cannot run .exe on linux
if (DEFINED LOCAL_C_INCLUDES)
  string(REPLACE " " ";" INCLUDES ${LOCAL_C_INCLUDES})
endif()

# Use same compiler config as android build system
set(CMAKE_C_FLAGS ${LOCAL_CFLAGS})
set(CMAKE_CXX_FLAGS  "${LOCAL_CFLAGS} ${LOCAL_CXXFLAGS}")

# Work around some gcc/mingw issues
if (NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if("${LOCAL_OS}" STREQUAL "windows_msvc")
  set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
endif()


# Make sure we create archives in the same way..
set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")

if (DEFINED LOCAL_INSTALL)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${LOCAL_INSTALL}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${LOCAL_INSTALL}")
endif()
# END COMPILER CONFIGURATION
# ==========================

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

if (WIN32 OR ("${LOCAL_OS_FLAVOR}" STREQUAL "windows"))
    # Work around some gcc/mingw issues
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_definitions(emulator-libext4_utils PRIVATE -DUSE_MINGW=1)
    endif ()
    if ("${LOCAL_OS}" STREQUAL "windows_msvc")
        target_include_directories(emulator-libext4_utils PRIVATE ${MSVC_POSIX_COMPAT_INCLUDE_DIR})
        target_include_directories(emulator-libext4_utils PRIVATE ${DIRENT_WIN32_INCLUDE_DIR})
        target_link_libraries(emulator-libext4_utils PRIVATE ${MSVC_POSIX_COMPAT_LIBRARY})
    endif()
endif ()
target_compile_options(emulator-libext4_utils PRIVATE -Wno-error)
target_compile_definitions(emulator-libext4_utils PRIVATE -DHOST)

