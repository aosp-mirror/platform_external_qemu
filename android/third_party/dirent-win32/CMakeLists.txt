cmake_minimum_required(VERSION 3.5)

# Dirent for windows-msvc.
if ("${ANDROID_TARGET_OS}" STREQUAL "windows_msvc")
    add_library(dirent-win32 include/dirent.h)
    target_include_directories(dirent-win32 PUBLIC include)
endif ()

set(DIRENT_WIN32_INCLUDE_DIR ${CURRENT_SOURCE_DIR}/include)
