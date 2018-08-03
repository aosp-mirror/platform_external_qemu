#
# A static library containing the automation protobuf generated code
#

# Compute AUTOMATIONPROTO_DIR relative to LOCAL_PATH

AUTOMATIONPROTO_DIR := $(call my-dir)
AUTOMATIONPROTO_DIR := $(AUTOMATIONPROTO_DIR:$(LOCAL_PATH)/%=%)

$(call start-emulator-library,libautomation_proto)

LOCAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS)

LOCAL_C_INCLUDES := \
    $(PROTOBUF_INCLUDES) \

LOCAL_PROTO_SOURCES := \
    $(AUTOMATIONPROTO_DIR)/automation.proto \

$(call end-emulator-library)

AUTOMATION_PROTO_STATIC_LIBRARIES := \
    libautomation_proto \
    $(PROTOBUF_STATIC_LIBRARIES) \
