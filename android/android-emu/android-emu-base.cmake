# This file defines android-emu-base library

# Dependencies
prebuilt(LZ4)
prebuilt(UUID)

# Source configuration, the following set is shared amongst all targets
set(android-emu-base_src
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
    android/utils/x86_cpuid.cpp)

# Windows 32-bit specific
set(android-emu-base_windows-x86_src
    android/base/files/preadwrite.cpp
    android/base/memory/SharedMemory_win32.cpp
    android/base/threads/Thread_win32.cpp
    android/base/system/Win32Utils.cpp
    android/base/system/Win32UnicodeString.cpp
    android/utils/win32_cmdline_quote.cpp
    android/utils/win32_unicode.cpp)

# Windows 64-bit (same as 32 bit)
set(android-emu-base_windows-x86_64_src ${android-emu-base_windows-x86_src})
set(android-emu-base_windows_msvc-x86_64_src ${android-emu-base_windows-x86_src})

# Mac specific sources
set(android-emu-base_darwin-x86_64_src
    android/base/memory/SharedMemory_posix.cpp
    android/base/threads/Thread_pthread.cpp
    android/base/system/system-native-mac.mm)

# Linux specific sources.
set(android-emu-base_linux-x86_64_src android/base/memory/SharedMemory_posix.cpp
    android/base/threads/Thread_pthread.cpp)

# Includes
set(android-emu-base_includes_private
    ${LZ4_INCLUDE_DIRS}
    ${UUID_INCLUDE_DIRS}
    .)

# Library dependencies, these are public so they will propagate
set(android-emu-base_libs_public ${LZ4_LIBRARIES} ${UUID_LIBRARIES})

if ("${ANDROID_TARGET_OS}" STREQUAL "windows_msvc")
    set(android-emu-base_includes_private
        ${android-emu-base_includes_private}
	${MSVC_POSIX_COMPAT_INCLUDE_DIR}
        ${DIRENT_WIN32_INCLUDE_DIR})
    set(android-emu-base_libs_public
        ${android-emu-base_libs_public}
	${MSVC_POSIX_COMPAT_LIBRARY})
endif()
# Compiler flags
set(android-emu-base_compile_options_private
    "-Wno-parentheses"
    "-Wno-invalid-constexpr"
    "-Wno-c++14-extensions")

add_android_library(android-emu-base)
