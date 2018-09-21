protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS verified_boot_config.proto)

add_library(verified_boot_proto ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(verified_boot_proto PUBLIC ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(verified_boot_proto PUBLIC ${PROTOBUF_LIBRARIES})
target_include_directories(verified_boot_proto PUBLIC ${CMAKE_BINARY_DIR})
