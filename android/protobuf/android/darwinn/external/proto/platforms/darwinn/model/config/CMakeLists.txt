# Defines the protobuf library featurecontrol
set(darwinnmodelconfig_src representation.proto array.proto internal.proto)
android_add_protobuf(darwinnmodelconfig "${darwinnmodelconfig_src}")