protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS snapshot.proto snapshot_deps.proto)

add_library(snapshot_proto ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(snapshot_proto PUBLIC ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(snapshot_proto PUBLIC ${PROTOBUF_LIBRARIES})
target_include_directories(snapshot_proto PUBLIC ${CMAKE_BINARY_DIR})
