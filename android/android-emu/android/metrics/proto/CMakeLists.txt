protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
    clientanalytics.proto
    device_restriction_enum.proto
    studio_stats.proto
    )

add_library(metrics_proto ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(metrics_proto PUBLIC ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(metrics_proto PUBLIC ${PROTOBUF_LIBRARIES})
target_include_directories(metrics_proto PUBLIC ${CMAKE_BINARY_DIR})
