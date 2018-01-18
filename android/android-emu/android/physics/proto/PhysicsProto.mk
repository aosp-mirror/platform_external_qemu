#
# A static library containing the physics protobuf generated code
#

# Compute PHYSICSPROTO_DIR relative to LOCAL_PATH

PHYSICSPROTO_DIR := $(call my-dir)
PHYSICSPROTO_DIR := $(PHYSICSPROTO_DIR:$(LOCAL_PATH)/%=%)

$(call start-emulator-library,libphysics_proto)

LOCAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS)

LOCAL_C_INCLUDES := \
    $(PROTOBUF_INCLUDES) \

LOCAL_PROTO_SOURCES := \
    $(PHYSICSPROTO_DIR)/physics.proto \

$(call end-emulator-library)

PHYSICS_PROTO_STATIC_LIBRARIES := \
    libphysics_proto \
    $(PROTOBUF_STATIC_LIBRARIES) \
