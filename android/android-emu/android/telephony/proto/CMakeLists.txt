protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS sim_access_rules.proto)

add_library(telephony_proto ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(telephony_proto PUBLIC ${PROTOBUF_INCLUDE_DIR})
target_link_libraries(telephony_proto PUBLIC ${PROTOBUF_LIBRARIES})
target_include_directories(telephony_proto PUBLIC ${CMAKE_BINARY_DIR})
