#
# A static library containing the snapshot protobuf generated code
#

# Compute SNAPSHOTPROTO_DIR relative to LOCAL_PATH

SNAPSHOTPROTO_DIR := $(call my-dir)
SNAPSHOTPROTO_DIR := $(SNAPSHOTPROTO_DIR:$(LOCAL_PATH)/%=%)

$(call start-emulator-library,libsnapshot_proto)

LOCAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS)

LOCAL_C_INCLUDES := \
    $(PROTOBUF_INCLUDES) \

LOCAL_PROTO_SOURCES := \
    $(SNAPSHOTPROTO_DIR)/snapshot.proto \
    $(SNAPSHOTPROTO_DIR)/snapshot_deps.proto \

$(call end-emulator-library)

SNAPSHOT_PROTO_STATIC_LIBRARIES := \
    libsnapshot_proto \
    $(PROTOBUF_STATIC_LIBRARIES) \
