# Protobuf support for the emulator code

PROTOBUF_TOP_DIR := $(PROTOBUF_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)
PROTOC_TOOL := $(PROTOBUF_PREBUILTS_DIR)/$(BUILD_HOST_TAG64)/bin/protoc

$(call define-emulator-prebuilt-library, \
    emulator-protobuf, \
    $(PROTOBUF_TOP_DIR)/lib/libprotobuf$(BUILD_TARGET_STATIC_LIBEXT))

PROTOBUF_INCLUDES := $(PROTOBUF_TOP_DIR)/include
PROTOBUF_STATIC_LIBRARIES := emulator-protobuf
