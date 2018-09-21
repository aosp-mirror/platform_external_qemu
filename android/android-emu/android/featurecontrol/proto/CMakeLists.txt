protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
    emulator_features.proto
    emulator_feature_patterns.proto
    )

add_library(featurecontrol_proto ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(featurecontrol_proto PUBLIC ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(featurecontrol_proto PUBLIC ${PROTOBUF_LIBRARIES})
target_include_directories(featurecontrol_proto PUBLIC ${CMAKE_BINARY_DIR})


