# This file defines android-emu-base library
# This is a very generic library, and should have no knowledge of the emulator whatsoever.
# This library should be useable by any other project.

# Dependencies
prebuilt(UUID)
prebuilt(GLIB2) # Acts as windows stdio compatibility layer.
prebuilt(LIBUNWIND)
prebuilt(TCMALLOC)
prebuilt(Threads)

# Source configuration, the following set is shared amongst all targets
set(android-emu-base_src
    android/base/AlignedBuf.cpp
    android/base/Backtrace.cpp
    android/base/ContiguousRangeMapper.cpp
    android/base/CpuTime.cpp
    android/base/CpuUsage.cpp
    android/base/Debug.cpp
    android/base/GLObjectCounter.cpp
    android/base/JsonWriter.cpp
    android/base/Log.cpp
    android/base/Pool.cpp
    android/base/ProcessControl.cpp
    android/base/Stopwatch.cpp
    android/base/StringFormat.cpp
    android/base/StringParse.cpp
    android/base/StringView.cpp
    android/base/SubAllocator.cpp
    android/base/Tracing.cpp
    android/base/Uri.cpp
    android/base/Uuid.cpp
    android/base/Version.cpp
    android/base/async/AsyncReader.cpp
    android/base/async/AsyncSocketServer.cpp
    android/base/async/AsyncWriter.cpp
    android/base/async/DefaultLooper.cpp
    android/base/async/Looper.cpp
    android/base/async/ScopedSocketWatch.cpp
    android/base/async/ThreadLooper.cpp
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
    android/base/gl_object_counter.cpp
    android/base/memory/LazyInstance.cpp
    android/base/memory/MemoryHints.cpp
    android/base/memory/MemoryTracker.cpp
    android/base/misc/FileUtils.cpp
    android/base/misc/HttpUtils.cpp
    android/base/misc/IpcPipe.cpp
    android/base/misc/StringUtils.cpp
    android/base/misc/Utf8Utils.cpp
    android/base/network/Dns.cpp
    android/base/network/IpAddress.cpp
    android/base/network/NetworkUtils.cpp
    android/base/perflogger/Benchmark.cpp
    android/base/perflogger/BenchmarkLibrary.cpp
    android/base/perflogger/Metric.cpp
    android/base/perflogger/WindowDeviationAnalyzer.cpp
    android/base/sockets/SocketDrainer.cpp
    android/base/sockets/SocketUtils.cpp
    android/base/sockets/SocketWaiter.cpp
    android/base/synchronization/MessageChannel.cpp
    android/base/system/System.cpp
    android/base/threads/Async.cpp
    android/base/threads/FunctorThread.cpp
    android/base/threads/ThreadStore.cpp
    android/base/threads/internal/ParallelTaskBase.cpp
    android/utils/bufprint.c
    android/utils/bufprint_system.cpp
    android/utils/debug.c
    android/utils/dirscanner.cpp
    android/utils/eintr_wrapper.c
    android/utils/fd.cpp
    android/utils/file_io.cpp
    android/utils/looper.cpp
    android/utils/misc.c
    android/utils/panic.c
    android/utils/path.cpp
    android/utils/path_system.cpp
    android/utils/socket_drainer.cpp
    android/utils/sockets.c
    android/utils/stralloc.c
    android/utils/system.c
    android/utils/tempfile.c)

# Windows 32-bit specific sources, these are only included in the windows 32 bit build
set(android-emu-base_windows_src
    android/base/files/preadwrite.cpp
    android/base/memory/SharedMemory_win32.cpp
    android/base/threads/Thread_win32.cpp
    android/base/system/Win32Utils.cpp
    android/base/system/Win32UnicodeString.cpp
    android/utils/win32_cmdline_quote.cpp
    android/utils/win32_unicode.cpp)

# Mac specific sources, only included in the darwin build.
set(android-emu-base_darwin-x86_64_src android/base/memory/SharedMemory_posix.cpp
    android/base/threads/Thread_pthread.cpp android/base/system/system-native-mac.mm)

# Linux specific sources.
set(android-emu-base_linux-x86_64_src android/base/memory/SharedMemory_posix.cpp
    android/base/threads/Thread_pthread.cpp)

# Let's add in the library
android_add_library(android-emu-base)

# Windows-msvc specific dependencies. Need these for posix support.
android_target_link_libraries(android-emu-base windows_msvc-x86_64 PUBLIC dirent-win32)

# Anyone who takes a dependency gets to use our header files.
target_include_directories(android-emu-base PUBLIC .)

# Library dependencies, these are public so they will propagate, if you link against the base you will link against LZ4
# & UUID
target_link_libraries(android-emu-base PRIVATE lz4 UUID::UUID PUBLIC libprotobuf Threads::Threads)
android_target_link_libraries(android-emu-base
                              linux-x86_64
                              PUBLIC
                              TCMALLOC::TCMALLOC
                              LIBUNWIND::LIBUNWIND
                              -ldl
                              -lrt)

android_target_link_libraries(android-emu-base
                              windows
                              PUBLIC
                              psapi::psapi
                              iphlpapi::iphlpapi
                              ws2_32::ws2_32)

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

# Compiler flags, note that these should never propagate (i.e. set to public) as we really want to limit the usage of
# these flags.
android_target_compile_options(android-emu-base Clang PRIVATE "-Wno-parentheses" "-Wno-invalid-constexpr")

# Add the benchmark
set(android-emu-base_benchmark_src android/base/synchronization/Lock_benchmark.cpp)
android_add_executable(android-emu-base_benchmark)
target_link_libraries(android-emu-base_benchmark PRIVATE android-emu-base emulator-gbench)

set(android-emu)

set(android-emu-base_unittests_src
    android/base/ArraySize_unittest.cpp
    android/base/AlignedBuf_unittest.cpp
    android/base/ContiguousRangeMapper_unittest.cpp
    android/base/async/Looper_unittest.cpp
    android/base/async/AsyncSocketServer_unittest.cpp
    android/base/async/RecurrentTask_unittest.cpp
    android/base/async/ScopedSocketWatch_unittest.cpp
    android/base/async/SubscriberList_unittest.cpp
    android/base/containers/BufferQueue_unittest.cpp
    android/base/containers/CircularBuffer_unittest.cpp
    android/base/containers/EntityManager_unittest.cpp
    android/base/containers/Lookup_unittest.cpp
    android/base/containers/SmallVector_unittest.cpp
    android/base/containers/StaticMap_unittest.cpp
    android/base/EintrWrapper_unittest.cpp
    android/base/files/FileShareOpen_unittest.cpp
    android/base/files/IniFile_unittest.cpp
    android/base/files/InplaceStream_unittest.cpp
    android/base/files/MemStream_unittest.cpp
    android/base/files/PathUtils_unittest.cpp
    android/base/files/ScopedFd_unittest.cpp
    android/base/files/ScopedStdioFile_unittest.cpp
    android/base/files/Stream_unittest.cpp
    android/base/files/StreamSerializing_unittest.cpp
    android/base/FunctionView_unittest.cpp
    android/base/JsonWriter_unittest.cpp
    android/base/Log_unittest.cpp
    android/base/memory/LazyInstance_unittest.cpp
    android/base/memory/MemoryHints_unittest.cpp
    android/base/memory/MallocUsableSize_unittest.cpp
    android/base/memory/OnDemand_unittest.cpp
    android/base/memory/ScopedPtr_unittest.cpp
    android/base/memory/SharedMemory_unittest.cpp
    android/base/misc/FileUtils_unittest.cpp
    android/base/misc/HttpUtils_unittest.cpp
    android/base/misc/IpcPipe_unittest.cpp
    android/base/misc/StringUtils_unittest.cpp
    android/base/misc/Utf8Utils_unittest.cpp
    android/base/network/Dns_unittest.cpp
    android/base/network/IpAddress_unittest.cpp
    android/base/network/NetworkUtils_unittest.cpp
    android/base/Optional_unittest.cpp
    android/base/perflogger/Benchmark_unittest.cpp
    android/base/Pool_unittest.cpp
    android/base/ProcessControl_unittest.cpp
    android/base/Result_unittest.cpp
    android/base/sockets/ScopedSocket_unittest.cpp
    android/base/sockets/SocketDrainer_unittest.cpp
    android/base/sockets/SocketUtils_unittest.cpp
    android/base/sockets/SocketWaiter_unittest.cpp
    android/base/StringFormat_unittest.cpp
    android/base/StringParse_unittest.cpp
    android/base/StringView_unittest.cpp
    android/base/SubAllocator_unittest.cpp
    android/base/synchronization/ConditionVariable_unittest.cpp
    android/base/synchronization/Lock_unittest.cpp
    android/base/synchronization/ReadWriteLock_unittest.cpp
    android/base/synchronization/MessageChannel_unittest.cpp
    android/base/system/System_unittest.cpp
    android/base/testing/MockUtils_unittest.cpp
    android/base/testing/TestEvent_unittest.cpp
    android/base/threads/Async_unittest.cpp
    android/base/threads/FunctorThread_unittest.cpp
    android/base/threads/ParallelTask_unittest.cpp
    android/base/threads/Thread_unittest.cpp
    android/base/threads/ThreadStore_unittest.cpp
    android/base/TypeTraits_unittest.cpp
    android/base/Uri_unittest.cpp
    android/base/Uuid_unittest.cpp
    android/base/Version_unittest.cpp
    android/utils/tempfile_unittest.cpp
    android/utils/bufprint_unittest.cpp
    android/utils/eintr_wrapper_unittest.cpp
    android/utils/dirscanner_unittest.cpp)

set(android-emu-base_unittests_windows_src android/base/files/ScopedFileHandle_unittest.cpp
    android/base/files/ScopedRegKey_unittest.cpp android/base/system/Win32UnicodeString_unittest.cpp
    android/base/system/Win32Utils_unittest.cpp)

# msvc specific unittests
set(android-emu-base_unittests_windows_msvc-x86_64_src android/base/system/WinMsvcSystem_unittest.cpp)

android_add_test(android-emu-base_unittests)
target_link_libraries(android-emu-base_unittests PRIVATE android-emu-base gmock_main)
