# Protobuf support for the emulator code

GRPC_TOP_DIR := $(GRPC_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

# Grpc plugin
PROTO_GRPC_PLUGIN = $(GRPC_PREBUILTS_DIR)/$(BUILD_HOST_TAG64)/bin/grpc_cpp_plugin

# We don't support grpc in 32 bit builds.
ifndef EMULATOR_BUILD_32BITS
    $(call define-emulator-prebuilt-library, \
        emulator-grpc, \
        $(GRPC_TOP_DIR)/lib/libgrpc_unsecure.a)

    $(call define-emulator-prebuilt-library, \
        emulator-grpc++, \
        $(GRPC_TOP_DIR)/lib/libgrpc++.a)

    GRPC_INCLUDES := $(GRPC_TOP_DIR)/include
    GRPC_STATIC_LIBRARIES := \
      emulator-grpc++ \
      emulator-grpc

endif


