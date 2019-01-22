# Defines the protobuf library verified-boot
set(verified-boot_src verified_boot_config.proto)
android_add_protobuf(verified-boot "${verified-boot_src}")
