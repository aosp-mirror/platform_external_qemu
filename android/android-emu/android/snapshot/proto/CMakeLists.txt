# Defines the protobuf library snapshot
set(snapshot_src snapshot.proto snapshot_deps.proto)
add_android_protobuf(snapshot "${snapshot_src}")
