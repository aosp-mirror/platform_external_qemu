#
# A static library containing the featurecontrol protobuf generated code
#

# Compute DARWINNMODELCONFIG_DIR relative to LOCAL_PATH
DARWINNMODELCONFIG_DIR := $(call my-dir)
DARWINNMODELCONFIG_DIR := $(DARWINNMODELCONFIG_DIR:$(LOCAL_PATH)/%=%)

$(call start-emulator-library,libdarwinnmodelconfig_proto)

LOCAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS)

LOCAL_C_INCLUDES := \
    $(PROTOBUF_INCLUDES) \
    $(DARWINNMODELCONFIG_DIR)/../../../.. \

LOCAL_PROTO_SOURCES := \
    $(DARWINNMODELCONFIG_DIR)/representation.proto \
    $(DARWINNMODELCONFIG_DIR)/array.proto \
    $(DARWINNMODELCONFIG_DIR)/internal.proto \

$(call end-emulator-library)

DARWINNMODELCONFIG_PROTO_INCLUDES := \
    $(PROTOBUF_INCLUDES) \
    $(DARWINNMODELCONFIG_DIR)/../../../.. \

DARWINNMODELCONFIG_PROTO_STATIC_LIBRARIES := \
    libdarwinnmodelconfig_proto \
    $(PROTOBUF_STATIC_LIBRARIES) \
