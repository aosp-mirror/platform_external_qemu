cmake_minimum_required(VERSION 3.5)
project(base-libraries)


# COMPILER + PATH CONFIGURATION
# =============================
# TODO(jansene): This section is needed as a bridge
# to the android build system.
string(REPLACE " " ";" INCLUDES ${LOCAL_C_INCLUDES})
include_directories(${INCLUDES})
include_directories(.)

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

set(COMMON_SOURCES
    android/base/ContiguousRangeMapper.cpp
    android/base/Debug.cpp
    android/base/files/CompressingStream.cpp
    android/base/files/DecompressingStream.cpp
    android/base/files/Fd.cpp
    android/base/files/FileShareOpen.cpp
    android/base/files/IniFile.cpp
    android/base/files/InplaceStream.cpp
    android/base/files/MemStream.cpp
    android/base/files/PathUtils.cpp
    android/base/files/StdioStream.cpp
    android/base/files/Stream.cpp
    android/base/files/StreamSerializing.cpp
    android/base/misc/FileUtils.cpp
    android/base/misc/HttpUtils.cpp
    android/base/misc/StringUtils.cpp
    android/base/misc/Utf8Utils.cpp
    android/base/network/IpAddress.cpp
    android/base/network/NetworkUtils.cpp
    android/base/Stopwatch.cpp
    android/base/StringFormat.cpp
    android/base/StringParse.cpp
    android/base/StringView.cpp
    android/base/sockets/SocketUtils.cpp
    android/base/sockets/SocketWaiter.cpp
    android/base/synchronization/MessageChannel.cpp
    android/base/Log.cpp
    android/base/memory/LazyInstance.cpp
    android/base/memory/MemoryHints.cpp
    android/base/ProcessControl.cpp
    android/base/system/System.cpp
    android/base/threads/Async.cpp
    android/base/threads/FunctorThread.cpp
    android/base/threads/ThreadStore.cpp
    android/base/Uri.cpp
    android/base/Uuid.cpp
    android/base/Version.cpp
    android/utils/aconfig-file.c
    android/utils/assert.c
    android/utils/async.cpp
    android/utils/bufprint.c
    android/utils/bufprint_system.cpp
    android/utils/cbuffer.c
    android/utils/debug.c
    android/utils/debug_wrapper.cpp
    android/utils/dll.c
    android/utils/dirscanner.cpp
    android/utils/eintr_wrapper.c
    android/utils/exec.cpp
    android/utils/fd.cpp
    android/utils/filelock.cpp
    android/utils/file_data.c
    android/utils/file_io.cpp
    android/utils/format.cpp
    android/utils/host_bitness.cpp
    android/utils/http_utils.cpp
    android/utils/iolooper.cpp
    android/utils/ini.cpp
    android/utils/intmap.c
    android/utils/ipaddr.cpp
    android/utils/lineinput.c
    android/utils/lock.cpp
    android/utils/mapfile.c
    android/utils/misc.c
    android/utils/panic.c
    android/utils/path.cpp
    android/utils/path_system.cpp
    android/utils/property_file.c
    android/utils/reflist.c
    android/utils/refset.c
    android/utils/sockets.c
    android/utils/stralloc.c
    android/utils/stream.cpp
    android/utils/string.cpp
    android/utils/system.c
    android/utils/system_wrapper.cpp
    android/utils/tempfile.c
    android/utils/timezone.cpp
    android/utils/uri.cpp
    android/utils/utf8_utils.cpp
    android/utils/vector.c
    android/utils/x86_cpuid.cpp
    )

# WIN32 Tag does not work on cross compilation, but will
# work when you are active on windows
if(WIN32 OR ("${LOCAL_OS}" STREQUAL "windows") )
    set(PLATFORM_SOURCES
        android/base/files/preadwrite.cpp
        android/base/memory/SharedMemory_win32.cpp
        android/base/threads/Thread_win32.cpp
        android/base/system/Win32Utils.cpp
        android/base/system/Win32UnicodeString.cpp
        android/utils/win32_cmdline_quote.cpp
        android/utils/win32_unicode.cpp
        )
else()
   set(PLATFORM_SOURCES
       android/base/memory/SharedMemory_posix.cpp
       android/base/threads/Thread_pthread.cpp
       )
endif()

if(APPLE)
    set (PLATFORM_SOURCES
        ${PLATFORM_SOURCES}
        android/base/system/system-native-mac.mm)
endif(APPLE)

add_library(android-emu-base ${COMMON_SOURCES} ${PLATFORM_SOURCES})
