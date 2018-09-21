protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
    automation.proto
    )

add_library(automation_proto ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(automation_proto PUBLIC ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(automation_proto PUBLIC ${PROTOBUF_LIBRARIES})
target_include_directories(automation_proto PUBLIC ${CMAKE_BINARY_DIR})

