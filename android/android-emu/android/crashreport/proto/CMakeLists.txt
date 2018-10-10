# Defines the protobuf library crashreport
set(crashreport_src crash_info.proto)
android_add_protobuf(crashreport "${crashreport_src}")
