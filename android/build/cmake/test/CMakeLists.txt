cmake_minimum_required(VERSION 3.5)
project(TEST)

set(TOOLS_DIRECTORY ${TEST_SOURCE_DIR}/../..)
list(APPEND CMAKE_MODULE_PATH "${TOOLS_DIRECTORY}/cmake")

include(prebuilts)
include(android)

# Must be c++14 or later for msvc build or clang will throw
# a lot of errors.
if(NOT ("${ANDROID_TARGET_OS}" STREQUAL "windows_msvc"))
  set(CMAKE_CXX_STANDARD 11)
endif ()

# These are not usually available outside our build system.
if(DEFINED ANDROID_TARGET_TAG)
  prebuilt(ANGLE)
  prebuilt(FFMPEG)
  prebuilt(BREAKPAD)
  prebuilt(LZ4)
  prebuilt(SDL2)
  prebuilt(UUID)
  prebuilt(VPX)
  # No x264 or virglrenderer on windows_msvc build
  if(NOT ("${ANDROID_TARGET_OS}" STREQUAL "windows_msvc"))
    prebuilt(X264)
    prebuilt(VIRGLRENDERER)
  endif()
  prebuilt(OpenSSL)
  prebuilt(QT5)
  # No usb on windows..
  if(NOT ("${ANDROID_TARGET_OS_FLAVOR}" STREQUAL "windows"))
    prebuilt(USB)
  endif()
endif()

prebuilt(CURL)
prebuilt(LIBXML2)
prebuilt(PNG)
prebuilt(Protobuf)
prebuilt(ZLIB)


# Let's define a shared hello lib and all its settings.
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)
set(hello_base_src  ${PROTO_SRCS} ${PROTO_HDRS})
set(hello_base_libs_public ${PROTOBUF_LIBRARIES})

# We want to propagate the protobuf headers we are creating..
set(hello_base_includes_public ${CMAKE_CURRENT_BINARY_DIR} ${PROTOBUF_INCLUDE_DIR})

# and declare the library
add_android_shared_library(hello_base)

# Next we define a hello executable, that uses our defined shared lib.

# This merely showcases how you can modify the sources
# for the various android targets, in our case we always use the same.
set(hello_darwin-x86_64_src hello.cpp)
set(hello_linux-x86_64_src hello.cpp)
set(hello_windows-x86_64_src hello.cpp)
set(hello_windows-x86_src hello.cpp)
# TODO: not sure how to add __dlspec(dllexport) to the generated protobuf
# headers, so just include the src for now.
set(hello_windows_msvc-x86_64_src hello.cpp ${PROTO_SRCS})

# Compiler options that should not propagate to the next target, we need -fPIC
# on lin/darwin for QT, on windows this is not needed.
set(hello_darwin-x86_64_compile_options_private -fPIC)
set(hello_linux-x86_64_compile_options_private  -fPIC)


# Libraries that should propagate if (PUBLIC) if dependencies would rely on this target
set(hello_libs_private hello_base ${UUID_LIBRARIES} ${QT5_LIBRARIES})

# Includes that are private
set(hello_includes_private  ${UUID_INCLUDE_DIR} ${QT5_INCLUDE_DIRS})

# The prebuilt dependencies.
set(hello_prebuilt_dependencies "${RUNTIME_OS_DEPENDENCIES};${QT5_SHARED_DEPENDENCIES}")
set(hello_prebuilt_properties "${RUNTIME_OS_PROPERTIES};${QT5_SHARED_PROPERTIES}")

# On the mac we have to make sure our rpath can find the shared lib.
set(hello_darwin-x86_64_prebuilt_properties "INSTALL_RPATH>=@loader_path")
add_android_executable(hello)

