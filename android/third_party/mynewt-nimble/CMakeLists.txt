project(Nimble)
cmake_minimum_required(VERSION 3.5)

if(NOT DEFINED ANDROID_TARGET_TAG)
  set(ANDROID_QEMU2_TOP_DIR ${CMAKE_CURRENT_LIST_DIR}/../../..)
  message(
    WARNING
      "You should invoke the cmake generator with a proper toolchain from ${ANDROID_QEMU2_TOP_DIR}/android/build/cmake, "
      "Trying to infer toolchain, this might not work.")
  list(APPEND CMAKE_MODULE_PATH "${ANDROID_QEMU2_TOP_DIR}/android/build/cmake/")
  include(toolchain)
  _get_host_tag(TAG)
  toolchain_configure_tags(${TAG})
endif()

# Let's check if we are building in our outside the emulator tree.
if(NOT ANDROID_QEMU_VERSION)
  message("Building outside of emulator build..")
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_C_STANDARD 11)
  enable_testing()
  include(android)
  include(prebuilts)
  prebuilt(Threads)
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/android/third_party/protobuf
                   protobuf)
  add_subdirectory(
    ${ANDROID_QEMU2_TOP_DIR}/android/third_party/perfetto-tracing-only-stub
    pstub)
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/android/third_party/lz4 lz4)
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/android/third_party/google-benchmark
                   google-benchmark)
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/android/third_party/googletest/
                   gtest)
  add_subdirectory(${ANDROID_QEMU2_TOP_DIR}/android/android-emu-base
                   android-emu-base)
  if(CMAKE_BUILD_TYPE MATCHES DEBUG AND NOT WINDOWS_MSVC_X86_64)
    # This will help you find issues.
    set(CMAKE_C_FLAGS "-fsanitize=address -fno-omit-frame-pointer -g3 -O0")
    set(CMAKE_EXE_LINKER_FLAGS "-fsanitize=address")
  endif()
endif()

message(STATUS "Including apache-nimble bluetooth stack.")
remove_definitions(
  "-DNDEBUG"
  "-DNDEBUG=1"
  "-D_FILE_OFFSET_BITS=64"
  "-D_GNU_SOURCE=1"
  "-D_LARGEFILE_SOURCE"
  "-D_LIBCPP_VERSION=__GLIBCPP__"
  "-D__STDC_CONSTANT_MACROS"
  "-D__STDC_FORMAT_MACROS=1"
  "-D__STDC_LIMIT_MACROS=1"
  "-DANDROID_DEBUG")

add_library(_nimble_os_abstraction_headers INTERFACE)
target_include_directories(
  _nimble_os_abstraction_headers
  INTERFACE # Change depending on os.
            porting/npl/android_emulator/include porting/nimble/include
            # Standard includes
            nimble/include)

add_subdirectory(porting)

add_subdirectory(ext/tinycrypt)
add_subdirectory(nimble/transport)

# Unlikely that you need a controller.
if(OPTION_WITH_NIMBLE_CONTROLLER)
  add_subdirectory(nimble/controller)
  add_subdirectory(nimble/drivers)
endif()

add_subdirectory(nimble/host)

# TODO(jansene): It would be nice if could select transport layers at runtime.
add_library(nimble_bluetooth INTERFACE)
target_link_libraries(
  nimble_bluetooth
  INTERFACE _nimble_os_abstraction_headers
            _nimble_os_abstraction
            _nimble_os_impl
            _nimble_store_ram
            _nimble_util
            _nimble_host_layer
            _nimble_service_layer
            _nimble_os_sysinit
            # _nimble_transport_socket # Use socket transport.
            _nimble_transport_grpc # Use the grpc transport
)

# Now include some sample apps
add_subdirectory(apps)

# Setup the unit tests.
add_subdirectory(testutil)
add_subdirectory(nimble/host/test)
