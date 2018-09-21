#
# A static library containing the offworld protobuf generated code
#

# Compute OFFWORLDPROTO_DIR relative to LOCAL_PATH

OFFWORLDPROTO_DIR := $(call my-dir)
OFFWORLDPROTO_DIR := $(OFFWORLDPROTO_DIR:$(LOCAL_PATH)/%=%)

$(call start-emulator-library,liboffworld_proto)

LOCAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS)

LOCAL_C_INCLUDES := \
    $(PROTOBUF_INCLUDES) \

LOCAL_PROTO_SOURCES := \
    $(OFFWORLDPROTO_DIR)/offworld.proto \

$(call end-emulator-library)

OFFWORLD_PROTO_STATIC_LIBRARIES := \
    liboffworld_proto \
    $(PROTOBUF_STATIC_LIBRARIES) \
