android_add_library(
  TARGET android-grpc-interceptors
  # SHARED # Testing only to make sure all dependencies are defined.
  LICENSE Apache-2.0
  SOURCE_DIR src
  SRC android/emulation/control/interceptor/IdleInterceptor.cpp
      android/emulation/control/interceptor/LoggingInterceptor.cpp
      android/emulation/control/interceptor/MetricsInterceptor.cpp
  DEPS android-emu-agents android-emu-base android-emu-base-headers
       android-emu-metrics protobuf::libprotobuf)
target_link_libraries(android-grpc-interceptors PUBLIC grpc++)
target_include_directories(android-grpc-interceptors PRIVATE src PUBLIC include)
target_compile_options(android-grpc-interceptors PUBLIC -Wno-extern-c-compat)

if(NOT WINDOWS_MSVC_X86_64)
  android_add_test(
    TARGET android-grpc-interceptors_unittest SOURCE_DIR test
    SRC android/emulation/control/interceptor/LoggingInterceptor_unittest.cpp
    DEPS android-grpc-interceptors emulator_controller-service gmock_main)
endif()

android_add_library(
  TARGET android-grpc-crash-interceptors
  # SHARED # Testing only to make sure all dependencies are defined.
  LICENSE Apache-2.0
  SOURCE_DIR src
  SRC android/emulation/control/interceptor/BreadcrumbInterceptor.cpp)
target_link_libraries(
  android-grpc-crash-interceptors PUBLIC android-emu-crashreport grpc++
                                         absl::strings android-emu-base-headers)
target_include_directories(android-grpc-crash-interceptors PRIVATE src
                           PUBLIC include)
target_compile_options(android-grpc-crash-interceptors
                       PUBLIC -Wno-extern-c-compat)
