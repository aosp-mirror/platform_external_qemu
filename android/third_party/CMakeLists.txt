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


# Make sure we create archives in the same way..
set(CMAKE_CXX_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> crs <TARGET> <LINK_FLAGS> <OBJECTS>")

if (DEFINED LOCAL_INSTALL)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${LOCAL_INSTALL}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${LOCAL_INSTALL}")
endif()
# END COMPILER CONFIGURATION
# ==========================



add_library(emulator-tinyobjloader ../../../tinyobjloader/tiny_obj_loader.cc)
target_include_directories(emulator-tinyobjloader PUBLIC ../../../tinyobjloader)

add_subdirectory(astc-codec)
add_subdirectory(jpeg-6b)
add_subdirectory(libdtb)
add_subdirectory(libselinux)
add_subdirectory(libsparse)
add_subdirectory(libwebp)
add_subdirectory(libyuv)
add_subdirectory(murmurhash)
add_subdirectory(tinyepoxy)
add_subdirectory(libkeymaster3)

if(WIN32 OR ("${LOCAL_OS}" STREQUAL "windows") )
  add_subdirectory(mman-win32)
endif()
