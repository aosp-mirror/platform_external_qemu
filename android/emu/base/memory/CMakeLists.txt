android_add_library(
  TARGET android-emu-base-memory
  # SHARED # Testing only
  LICENSE Apache-2.0 SOURCE_DIR
  src
  SRC android/base/memory/ContiguousRangeMapper.cpp
      android/base/memory/LazyInstance.cpp android/base/memory/MemoryHints.cpp
      android/base/memory/MemoryTracker.cpp
  POSIX android/base/memory/SharedMemory_posix.cpp
  WINDOWS android/base/memory/SharedMemory_win32.cpp
          # android/base/memory/Win32UnicodeString.cpp
          # android/base/memory/Win32Utils.cpp
  DEPS android-emu-base-headers)

if(OPTION_TCMALLOC)
  android_target_link_libraries(
    android-emu-base-memory linux PUBLIC TCMALLOC::TCMALLOC LIBUNWIND::LIBUNWIND
                                    -ldl Threads::Threads -lrt)
  target_compile_definitions(android-emu-base-memory
                             PRIVATE "AEMU_TCMALLOC_ENABLED=1")
else()
  android_target_link_libraries(
    android-emu-base-memory linux PUBLIC LIBUNWIND::LIBUNWIND -ldl Threads::Threads
                                    -lrt)
  target_compile_definitions(android-emu-base-memory
                             PUBLIC "AEMU_TCMALLOC_ENABLED=0")
endif()

target_include_directories(android-emu-base-memory PRIVATE src)
target_link_libraries(android-emu-base-memory PUBLIC android-emu-base-logging)
android_add_test(
  TARGET android-emu-base-memory_unittests
  SOURCE_DIR test
  SRC android/base/memory/ContiguousRangeMapper_unittest.cpp
      android/base/memory/LazyInstance_unittest.cpp
      android/base/memory/MallocUsableSize_unittest.cpp
      android/base/memory/MemoryHints_unittest.cpp
      android/base/memory/OnDemand_unittest.cpp
      android/base/memory/ScopedPtr_unittest.cpp
      android/base/memory/SharedMemory_unittest.cpp
  DEPS android-emu-base android-emu-base-headers android-emu-base-memory gmock_main)
target_include_directories(android-emu-base-memory_unittests PRIVATE src)
