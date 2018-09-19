cmake_minimum_required(VERSION 3.5)
project(TEST)


SET (TOOLS_DIRECTORY ${TEST_SOURCE_DIR}/android/build)
LIST (APPEND CMAKE_MODULE_PATH "${TOOLS_DIRECTORY}/cmake")


include(prebuilts)

set(LOCAL_TARGET_TAG "linux-x86_64")
set(LOCAL_QEMU2_TOP_DIR ".")
prebuilt(CURL)
prebuilt(PNG)
prebuilt(ZLIB)
prebuilt(Protobuf)
prebuilt(ANGLE)
message("Using ${PROTOBUF_LIBRARIES}")
#prebuilt(Qt5)
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)
# prebuilt(SDL2)

message("Source: ${PROTO_SRCS} Header: ${PROTO_HDRS} [${PROTOBUF_INCLUDE_DIRS}]")
include_directories(${PROTOBUF_INCLUDE_DIRS})
add_executable(hello hello.c ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(hello ${PROTOBUF_LIBRARIES})
target_include_directories(hello PRIVATE ${PROTOBUF_INCLUDE_DIRS})
