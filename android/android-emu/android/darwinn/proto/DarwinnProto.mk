#
# A static library containing the offworld protobuf generated code
#

# Compute OFFWORLDPROTO_DIR relative to LOCAL_PATH

DARWINNPROTO_DIR := $(call my-dir)
DARWINNPROTO_DIR := $(DARWINNPROTO_DIR:$(LOCAL_PATH)/%=%)

$(call start-emulator-library,libdarwinn_proto)

LOCAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS)

LOCAL_C_INCLUDES := \
    $(PROTOBUF_INCLUDES) \

LOCAL_PROTO_SOURCES := \
    $(DARWINNPROTO_DIR)/darwinnpipe.proto \

$(call end-emulator-library)

DARWINN_PROTO_STATIC_LIBRARIES := \
    libdarwinn_proto \
    $(PROTOBUF_STATIC_LIBRARIES) \
