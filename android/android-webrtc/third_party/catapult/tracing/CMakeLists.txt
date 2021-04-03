add_library(webrtc_histogram_proto_bridge)
protobuf_generate_with_plugin(
  TARGET webrtc_histogram_proto_bridge PROTOS ./tracing/proto/histogram.proto
  APPEND_PATH PROTOPATH -I${CMAKE_CURRENT_SOURCE_DIR}/tracing/proto/
  PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/tracing/tracing/proto/)
target_include_directories(webrtc_histogram_proto_bridge
                           PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(webrtc_histogram_proto_bridge PUBLIC libprotobuf)

add_library(webrtc_catapult_reserved_infos
            tracing/value/diagnostics/reserved_infos.cc)
target_include_directories(webrtc_catapult_reserved_infos PUBLIC ..
                           ../../..)
target_link_libraries(webrtc_catapult_reserved_infos
                      PUBLIC webrtc_histogram_proto_bridge)
add_library(webrtc_catapult_histogram tracing/value/histogram.cc
                                      tracing/value/running_statistics.cc)
target_include_directories(webrtc_catapult_histogram PRIVATE .. PUBLIC ../../..)
target_link_libraries(webrtc_catapult_histogram
                      PUBLIC webrtc_histogram_proto_bridge)
