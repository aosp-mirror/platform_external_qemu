# Protobuf support for the emulator code

GRPC_TOP_DIR := $(GRPC_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

# Grpc plugin
PROTO_GRPC_PLUGIN = $(GRPC_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/bin/grpc_cpp_plugin

$(call define-emulator-prebuilt-library, \
    emulator-grpc, \
    $(GRPC_TOP_DIR)/lib/libgrpc_unsecure.a)

$(call define-emulator-prebuilt-library, \
    emulator-grpc++, \
    $(GRPC_TOP_DIR)/lib/libgrpc++.a)

$(call define-emulator-prebuilt-library, \
    emulator-grpc-reflection, \
    $(GRPC_TOP_DIR)/lib/libgrpc++_reflection.a)

GRPC_INCLUDES := $(GRPC_TOP_DIR)/include
GRPC_STATIC_LIBRARIES := \
  emulator-grpc \
  emulator-grpc++ \
  emulator-grpc-reflection

