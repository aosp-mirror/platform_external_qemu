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

# This is a posix wrapper for windows-msvc build.
if ("${ANDROID_TARGET_OS}" STREQUAL "windows_msvc")
    add_library(msvc-posix-compat
            src/msvc-posix.c
            src/getopt.c
            src/gettimeofday.c)
    target_include_directories(msvc-posix-compat PUBLIC include)
endif ()

set(MSVC_POSIX_COMPAT_INCLUDE_DIR ${CURRENT_SOURCE_DIR}/include)
set(MSVC_POSIX_COMPAT_LIBRARY ${CMAKE_BINARY_DIR}/msvc-posix-compat.lib)
