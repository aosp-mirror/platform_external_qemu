cmake_minimum_required(VERSION 3.5)
project(TEST)


SET (TOOLS_DIRECTORY ${TEST_SOURCE_DIR}/../..)
LIST (APPEND CMAKE_MODULE_PATH "${TOOLS_DIRECTORY}/cmake")


include(prebuilts)

# These are not usually available outside our build system.
if (DEFINED LOCAL_TARGET_TAG)
    prebuilt(ANGLE)
	prebuilt(FFMPEG)
    prebuilt(BREAKPAD)
    prebuilt(LZ4)
    prebuilt(SDL2)
    prebuilt(UUID)
    prebuilt(VIRGLRENDERER)
    prebuilt(VPX)
    prebuilt(X264)
    # No usb on windows..
    if (NOT ("${LOCAL_TARGET_TAG}" MATCHES "windows.*") )
        prebuilt(USB)
    endif ()
endif ()

prebuilt(CURL)
prebuilt(LIBXML2)
prebuilt(OpenSSL)
prebuilt(PNG)
prebuilt(Protobuf)
prebuilt(ZLIB)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS test.proto)
add_executable(hello hello.c ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(hello ${PROTOBUF_LIBRARIES})
target_link_libraries(hello ${UUID_LIBRARIES})

target_include_directories(hello PRIVATE ${PROTOBUF_INCLUDE_DIR})
target_include_directories(hello PRIVATE ${UUID_INCLUDE_DIR})
