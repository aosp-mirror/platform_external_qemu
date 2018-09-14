cmake_minimum_required(VERSION 3.5)

# COMPILER + PATH CONFIGURATION
# =============================

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


add_library(emulator-libdtb src/libdtb.c)

target_include_directories(emulator-libdtb PUBLIC include)
target_include_directories(emulator-libdtb PRIVATE .)
target_compile_definitions(emulator-libdtb PRIVATE -DHOST)
target_compile_options(emulator-libdtb PRIVATE -Wno-error)

if(WIN32 OR ("${LOCAL_OS}" STREQUAL "windows") )
    # Work around some gcc/mingw issues
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_definitions(emulator-libdtb PRIVATE -DUSE_MINGW=1)
    endif()
endif()

