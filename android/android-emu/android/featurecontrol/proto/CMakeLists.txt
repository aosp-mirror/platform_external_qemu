# Defines the protobuf library featurecontrol
set(featurecontrol_src emulator_feature_patterns.proto emulator_features.proto)
add_android_protobuf(featurecontrol "${featurecontrol_src}")
