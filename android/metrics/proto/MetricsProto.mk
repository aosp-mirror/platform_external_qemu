#
# A static library containing the metrics protobuf generated code
#

# Grab the current directory
METRICS_DIR := $(call my-dir)

$(call start-emulator-library,libmetrics_proto)

LOCAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS)

LOCAL_C_INCLUDES := \
    $(PROTOBUF_INCLUDES) \

LOCAL_PROTO_SOURCES := \
    $(METRICS_DIR)/clientanalytics.proto \
    $(METRICS_DIR)/studio_stats.proto \

$(call end-emulator-library)

METRICS_PROTO_INCLUDES := $(PROTOBUF_INCLUDES)
METRICS_PROTO_STATIC_LIBRARIES := \
    libmetrics_proto \
    $(PROTOBUF_STATIC_LIBRARIES)
