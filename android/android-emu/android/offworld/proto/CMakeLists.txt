protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS offworld.proto)

add_library(offworld_proto ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(offworld_proto PUBLIC ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(offworld_proto PUBLIC ${PROTOBUF_LIBRARIES})
target_include_directories(offworld_proto PUBLIC ${CMAKE_BINARY_DIR})
