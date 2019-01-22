# Defines the protobuf library telephony
set(telephony_src sim_access_rules.proto)
android_add_protobuf(telephony "${telephony_src}")
