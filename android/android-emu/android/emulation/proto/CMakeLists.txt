# Defines the protobuf library emulation
set(emulation_src VehicleHalProto.proto observation.proto)
android_add_protobuf(emulation "${emulation_src}")
