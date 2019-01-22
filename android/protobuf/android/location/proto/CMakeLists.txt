# Defines the protobuf library location
set(location_src point.proto route.proto)
android_add_protobuf(location "${location_src}")
