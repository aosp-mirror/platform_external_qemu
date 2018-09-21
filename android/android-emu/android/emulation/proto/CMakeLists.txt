protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS
    VehicleHalProto.proto)

add_library(emulation_proto ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(emulation_proto PUBLIC ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(emulation_proto PUBLIC ${PROTOBUF_LIBRARIES})
target_include_directories(emulation_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/../../..)

