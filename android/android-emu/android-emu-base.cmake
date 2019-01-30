# This file defines android-emu-base library
# This is a library that has very few dependencies (and we would like to keep it like that)

# Dependencies
prebuilt(UUID)

# Source configuration, the following set is shared amongst all targets
set(ANDROID_EMU_BASE_SOURCES
    android/base/async/AsyncReader.cpp
    android/base/async/AsyncSocketServer.cpp
    android/base/async/AsyncWriter.cpp
    android/base/async/DefaultLooper.cpp
    android/base/async/Looper.cpp
    android/base/async/ScopedSocketWatch.cpp
    android/base/async/ThreadLooper.cpp
    android/base/network/Dns.cpp
    android/base/sockets/SocketDrainer.cpp
    android/base/sockets/SocketUtils.cpp
    android/base/sockets/SocketWaiter.cpp
    android/base/threads/internal/ParallelTaskBase.cpp
    android/base/AlignedBuf.cpp
    android/base/Backtrace.cpp
    android/base/ContiguousRangeMapper.cpp
    android/base/CpuTime.cpp
    android/base/CpuUsage.cpp
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
    android/base/misc/IpcPipe.cpp
    android/base/misc/StringUtils.cpp
    android/base/misc/Utf8Utils.cpp
    android/base/network/IpAddress.cpp
    android/base/network/NetworkUtils.cpp
    android/base/Pool.cpp
    android/base/Stopwatch.cpp
    android/base/StringFormat.cpp
    android/base/StringParse.cpp
    android/base/StringView.cpp
    android/base/SubAllocator.cpp
    android/base/synchronization/MessageChannel.cpp
    android/base/JsonWriter.cpp
    android/base/Log.cpp
    android/base/memory/LazyInstance.cpp
    android/base/memory/MemoryHints.cpp
    android/base/perflogger/Benchmark.cpp
    android/base/perflogger/BenchmarkLibrary.cpp
    android/base/perflogger/Metric.cpp
    android/base/perflogger/WindowDeviationAnalyzer.cpp
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
    android/utils/Random.cpp
    android/utils/sockets.c
    android/utils/socket_drainer.cpp
    android/utils/looper.cpp)

# Windows 32-bit specific sources, these are only included in the windows 32 bit build
set(ANDROID_EMU_BASE_WINDOWS_SOURCES
    android/base/files/preadwrite.cpp
    android/base/memory/SharedMemory_win32.cpp
    android/base/threads/Thread_win32.cpp
    android/base/system/Win32Utils.cpp
    android/base/system/Win32UnicodeString.cpp
    android/utils/win32_cmdline_quote.cpp
    android/utils/win32_unicode.cpp)

# Mac specific sources, only included in the darwin build.
set(ANDROID_EMU_BASE_DARWIN_SOURCES
    android/base/memory/SharedMemory_posix.cpp
    android/base/threads/Thread_pthread.cpp
    android/base/system/system-native-mac.mm)

# Linux specific sources.
set(ANDROID_EMU_BASE_LINUX_SOURCES
    android/base/memory/SharedMemory_posix.cpp
    android/base/threads/Thread_pthread.cpp)

# The static library (to be removed)

set(android-emu-base_src ${ANDROID_EMU_BASE_SOURCES})
set(android-emu-base_windows_src ${ANDROID_EMU_BASE_WINDOWS_SOURCES})
set(android-emu-base_darwin-x86_64_src ${ANDROID_EMU_BASE_DARWIN_SOURCES})
set(android-emu-base_linux-x86_64_src android/base/memory/SharedMemory_posix.cpp
    android/base/threads/Thread_pthread.cpp)

# Let's add in the library
android_add_library(android-emu-base)

# Windows-msvc specific dependencies. Need these for posix support.
android_target_link_libraries(android-emu-base windows_msvc PUBLIC msvc-posix-compat dirent-win32)

# Anyone who takes a dependency gets to use our header files.
target_include_directories(android-emu-base PUBLIC .)

# Library dependencies, these are public so they will propagate, if you link against the base you will link against LZ4
# & UUID
target_link_libraries(android-emu-base PRIVATE lz4 UUID::UUID)
android_target_link_libraries(android-emu-base linux-x86_64 PUBLIC -ldl Threads::Threads -lrt)
android_target_link_libraries(android-emu-base windows-x86_64 PUBLIC psapi::psapi Threads::Threads iphlpapi::iphlpapi)
android_target_link_libraries(android-emu-base
                              darwin-x86_64
                              PUBLIC
                              "-framework Foundation"
                              "-framework ApplicationServices"
                              "-framework IOKit")
android_target_compile_definitions(android-emu-base
                                   darwin-x86_64
                                   PRIVATE
                                   "-D_DARWIN_C_SOURCE=1"
                                   "-Dftello64=ftell"
                                   "-Dfseeko64=fseek")

# Compiler flags, not that these should never propagate (i.e. set to public) as we really want to limit the usage of
# these flags.
android_target_compile_options(android-emu-base Clang
                               PRIVATE
                               "-Wno-parentheses"
                               "-Wno-invalid-constexpr")

# Add the benchmark
set(android-emu_benchmark_src android/base/synchronization/Lock_benchmark.cpp)
android_add_executable(android-emu_benchmark)
target_link_libraries(android-emu_benchmark PRIVATE android-emu-base emulator-gbench)

# The shared library
set(android-emu-base-shared_src ${ANDROID_EMU_BASE_SOURCES})
set(android-emu-base-shared_src ${ANDROID_EMU_BASE_SOURCES})
set(android-emu-base-shared_windows_src ${ANDROID_EMU_BASE_WINDOWS_SOURCES})
set(android-emu-base-shared_darwin-x86_64_src ${ANDROID_EMU_BASE_DARWIN_SOURCES})
set(android-emu-base-shared_linux-x86_64_src android/base/memory/SharedMemory_posix.cpp
    android/base/threads/Thread_pthread.cpp)

# Let's add in the library
android_add_shared_library(android-emu-base-shared)

# Windows-msvc specific dependencies. Need these for posix support.
android_target_link_libraries(android-emu-base-shared windows_msvc PUBLIC msvc-posix-compat dirent-win32)

# Anyone who takes a dependency gets to use our header files.
target_include_directories(android-emu-base-shared PUBLIC .)

# Library dependencies, these are public so they will propagate, if you link against the base you will link against LZ4
# & UUID
target_link_libraries(android-emu-base-shared PRIVATE lz4 UUID::UUID)
android_target_link_libraries(android-emu-base-shared linux-x86_64 PUBLIC -ldl Threads::Threads -lrt)
android_target_link_libraries(android-emu-base-shared windows-x86_64 PUBLIC psapi::psapi Threads::Threads iphlpapi::iphlpapi)
android_target_link_libraries(android-emu-base-shared
                              darwin-x86_64
                              PUBLIC
                              "-framework Foundation"
                              "-framework ApplicationServices"
                              "-framework IOKit")
android_target_link_libraries(android-emu-base-shared windows PRIVATE ws2_32::ws2_32)
android_target_compile_definitions(android-emu-base-shared
                                   darwin-x86_64
                                   PRIVATE
                                   "-D_DARWIN_C_SOURCE=1"
                                   "-Dftello64=ftell"
                                   "-Dfseeko64=fseek")

# Compiler flags, not that these should never propagate (i.e. set to public) as we really want to limit the usage of
# these flags.
target_compile_options(android-emu-base-shared
                       PRIVATE
                       -fvisibility=default)
android_target_compile_options(android-emu-base-shared Clang
                               PRIVATE
                               "-Wno-parentheses"
                               "-Wno-invalid-constexpr")
