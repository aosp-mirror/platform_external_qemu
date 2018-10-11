# Defines the protobuf library emulation
set(emulation_src VehicleHalProto.proto)
add_android_protobuf(emulation "${emulation_src}")
