protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
    point.proto
    route.proto
    )

add_library(location_proto ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(location_proto PUBLIC ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(location_proto PUBLIC ${PROTOBUF_LIBRARIES})
target_include_directories(location_proto PUBLIC ${CMAKE_BINARY_DIR})

