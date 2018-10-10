# Defines the protobuf library snapshot
set(snapshot_src snapshot.proto snapshot_deps.proto)
android_add_protobuf(snapshot "${snapshot_src}")
