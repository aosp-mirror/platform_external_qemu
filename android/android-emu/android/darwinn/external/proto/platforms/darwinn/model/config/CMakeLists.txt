# Defines the protobuf library featurecontrol
set(darwinnmodelconfig_src representation.proto array.proto internal.proto)
add_android_protobuf(darwinnmodelconfig "${darwinnmodelconfig_src}")