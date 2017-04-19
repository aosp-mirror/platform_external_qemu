list(APPEND SOURCES
  #feature_control.cpp
  #FeatureControl.cpp
  #FeatureControl_unittest.cpp
  #FeatureControlImpl.cpp
  #HWMatching.cpp
  #HWMatching_unittest.cpp
  )


if(APPLE)
elseif(UNIX)
elseif(WIN32)
endif()
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS proto/emulator_feature_patterns.proto)

add_library(featurecontrol ${SOURCES}  ${PROTO_SRCS} ${PROTO_HDRS})
