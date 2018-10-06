# Defines the protobuf library location
set(location_src point.proto route.proto)
add_android_protobuf(location "${location_src}")
