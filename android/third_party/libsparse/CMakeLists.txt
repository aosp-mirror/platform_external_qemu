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

add_library(emulator-libsparse
    src/backed_block.c
    src/output_file.c
    src/sparse.c
    src/sparse_crc32.c
    src/sparse_err.c
    src/sparse_read.c
    )

target_include_directories(emulator-libsparse PUBLIC include)
target_include_directories(emulator-libsparse PRIVATE src)

if(WIN32 OR ("${LOCAL_OS}" STREQUAL "windows") )
    # Work around some gcc/mingw issues
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_definitions(emulator-libsparse PRIVATE -DUSE_MINGW=1)
    endif()
endif()

target_compile_options(emulator-libsparse PRIVATE -Wno-error)

