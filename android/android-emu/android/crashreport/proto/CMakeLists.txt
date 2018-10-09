# Defines the protobuf library crashreport
set(crashreport_src crash_info.proto)
add_android_protobuf(crashreport "${crashreport_src}")
