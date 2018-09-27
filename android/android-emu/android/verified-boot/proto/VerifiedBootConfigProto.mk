#
# A static library containing the verifiedbootcfg protobuf generated code
#

# Compute VERIFIEDBOOTCFG_DIR relative to LOCAL_PATH
VERIFIEDBOOTCFG_DIR := $(call my-dir)
VERIFIEDBOOTCFG_DIR := $(VERIFIEDBOOTCFG_DIR:$(LOCAL_PATH)/%=%)

$(call start-emulator-library,libverifiedbootcfg_proto)

LOCAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS)

LOCAL_C_INCLUDES := \
    $(PROTOBUF_INCLUDES) \

LOCAL_PROTO_SOURCES := \
    $(VERIFIEDBOOTCFG_DIR)/verified_boot_config.proto \

$(call end-emulator-library)

VERIFIEDBOOTCFG_PROTO_INCLUDES := $(PROTOBUF_INCLUDES)
VERIFIEDBOOTCFG_PROTO_STATIC_LIBRARIES := \
    libverifiedbootcfg_proto \
    $(PROTOBUF_STATIC_LIBRARIES) \
