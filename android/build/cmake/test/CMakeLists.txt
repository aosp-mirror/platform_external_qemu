cmake_minimum_required(VERSION 3.5)
project(TEST)

set(TOOLS_DIRECTORY ${TEST_SOURCE_DIR}/../..)
list(APPEND CMAKE_MODULE_PATH "${TOOLS_DIRECTORY}/cmake")

include(prebuilts)

set(CMAKE_CXX_STANDARD 11)

# These are not usually available outside our build system.
if(DEFINED ANDROID_TARGET_TAG)
  prebuilt(ANGLE)
  prebuilt(FFMPEG)
  prebuilt(BREAKPAD)
  prebuilt(LZ4)
  prebuilt(SDL2)
  prebuilt(UUID)
  prebuilt(VIRGLRENDERER)
  prebuilt(VPX)
  prebuilt(X264)
  prebuilt(OpenSSL)
  prebuilt(QT5)
  # No usb on windows..
  if(NOT ("${ANDROID_TARGET_TAG}" MATCHES "windows.*"))
    prebuilt(USB)
  endif()
endif()

prebuilt(CURL)
prebuilt(LIBXML2)
prebuilt(PNG)
prebuilt(Protobuf)
prebuilt(ZLIB)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)
add_executable(hello hello.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_compile_options(hello PRIVATE -fPIC)
target_link_libraries(hello
                      ${PROTOBUF_LIBRARIES}
                      ${UUID_LIBRARIES}
                      ${QT5_LIBRARIES})

target_include_directories(hello
                           PRIVATE ${PROTOBUF_INCLUDE_DIR} ${UUID_INCLUDE_DIR}
                                   ${QT5_INCLUDE_DIRS})
target_prebuilt_dependency(hello "${RUNTIME_OS_DEPENDENCIES}"
                           "${RUNTIME_OS_PROPERTIES}")
target_prebuilt_dependency(hello "${QT5_SHARED_DEPENDENCIES}"
                           "${QT5_SHARED_PROPERTIES}")
