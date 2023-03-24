android_add_library(
  TARGET android-waterfall-server-impl
  # SHARED # Testing only to make sure all dependencies are defined.
  LICENSE Apache-2.0
  SOURCE_DIR
  src
  SRC android/emulation/control/waterfall/WaterfallFactory.cpp
  POSIX android/emulation/control/waterfall/AdbWaterfallService.cpp
        android/emulation/control/waterfall/SocketController.cpp
        android/emulation/control/waterfall/WaterfallService.cpp
  DEPS grpc++)

if(NOT WINDOWS_MSVC_X86_64)
  target_link_libraries(
    android-waterfall-server-impl
    PUBLIC waterfall-service
           android-emu-adb-interface
           android-emu-base-headers
           android-emu-hardware
           android-grpc-interceptors
           png
           qemu-host-common-headers)
endif()
target_include_directories(android-waterfall-server-impl PUBLIC include
                           PRIVATE src)
target_compile_options(android-waterfall-server-impl
                       PRIVATE -Wno-return-type-c-linkage)
