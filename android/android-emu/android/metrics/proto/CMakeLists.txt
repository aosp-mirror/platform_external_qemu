PROTOBUF_GENERATE_CPP(CLIENTANALYTICS_SRC CLIENTANALYTICS_HDRS clientanalytics.proto)
PROTOBUF_GENERATE_CPP(DEVICE_RESTRICTION_ENUM_SRC DEVICE_RESTRICTION_ENUM_HDRS device_restriction_enum.proto)
PROTOBUF_GENERATE_CPP(STUDIO_STATS_SRC STUDIO_STATS_HDRS studio_stats.proto)

include_directories(
  ${Protobuf_INCLUDE_DIR}
  ${CONFIG_HOST_INCLUDE_DIRS}
  )


list(APPEND METRICS_PROTO_SOURCES
  ${CLIENTANALYTICS_SRC}
  ${CLIENTANALYTICS_HDRS}
  ${DEVICE_RESTRICTION_ENUM_SRC}
  ${DEVICE_RESTRICTION_ENUM_HDRS}
  ${STUDIO_STATS_SRC}
  ${STUDIO_STATS_HDRS}
  )


add_library(metrics_proto ${METRICS_PROTO_SOURCES})
target_link_libraries(metrics_proto PUBLIC ${Protobuf_LIBRARIES})


# Copy the headers to a location that matches with the Android.mk build system.
add_custom_command(TARGET metrics_proto POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CLIENTANALYTICS_HDRS}
  ${CMAKE_BINARY_DIR}/android/metrics/proto/clientanalytics.pb.h
)
add_custom_command(TARGET metrics_proto POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${DEVICE_RESTRICTION_ENUM_HDRS}
  ${CMAKE_BINARY_DIR}/android/metrics/proto/device_restriction_enum.pb.h
)
add_custom_command(TARGET metrics_proto POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${STUDIO_STATS_HDRS}
  ${CMAKE_BINARY_DIR}/android/metrics/proto/studio_stats.pb.h
)
