PROTOBUF_GENERATE_CPP(FEATURE_PATTERNS_SRC FEATURE_PATTERNS_HDRS emulator_feature_patterns.proto)
PROTOBUF_GENERATE_CPP(FEATURE_SRC FEATURE_HDRS emulator_features.proto)

include_directories(${Protobuf_INCLUDE_DIR})
add_library(featurecontrol_proto ${FEATURE_PATTERNS_SRC} ${FEATURE_SRC} ${FEATURE_PATTERNS_HDRS} ${FEATURE_HDRS})
