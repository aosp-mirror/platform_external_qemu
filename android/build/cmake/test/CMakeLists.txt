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

# and declare the library
android_add_shared_library(hello_base)

target_link_libraries(hello_base PUBLIC ${PROTOBUF_LIBRARIES})

# We want to propagate the protobuf headers we are creating..
target_include_directories(hello_base PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${PROTOBUF_INCLUDE_DIRS})


# This merely showcases how you can modify the sources
# for the various android targets, in our case we always use the same.
set(hello_darwin-x86_64_src hello.cpp)
set(hello_linux-x86_64_src hello.cpp)
set(hello_windows-x86_64_src hello.cpp)
set(hello_windows-x86_src hello.cpp)
# TODO: not sure how to add __dlspec(dllexport) to the generated protobuf
# headers, so just include the src for now.
set(hello_windows_msvc-x86_64_src hello.cpp ${PROTO_SRCS})

# Add the executable
android_add_executable(hello)

# Compiler options that should not propagate to the next target, we need -fPIC
# on lin/darwin for QT, on windows this is not needed.
# Note that this command is basically the same as the CMake
# target_compile_options, with the difference that the 2nd argument
# specifies the android target.
android_target_compile_options(hello darwin-x86_64 PRIVATE -fPIC)
android_target_compile_options(hello linux-x86_64 PRIVATE  -fPIC)


# Libraries that should propagate if (PUBLIC) if dependencies would rely on this target
target_link_libraries(hello PRIVATE  hello_base ${UUID_LIBRARIES} ${QT5_LIBRARIES})

# Includes that are private
target_include_directories(hello PRIVATE ${UUID_INCLUDE_DIR} ${QT5_INCLUDE_DIRS})

# The prebuilt dependencies.
android_target_dependency(hello all "${RUNTIME_OS_DEPENDENCIES};${QT5_SHARED_DEPENDENCIES}")
android_target_properties(hello all "${RUNTIME_OS_PROPERTIES};${QT5_SHARED_PROPERTIES}")