#
# A static library containing the sim_access_rules protobuf generated code
#

# Compute SIM_ACCESS_RULES_DIR relative to LOCAL_PATH
SIM_ACCESS_RULES_DIR := $(call my-dir)
SIM_ACCESS_RULES_DIR := $(SIM_ACCESS_RULES_DIR:$(LOCAL_PATH)/%=%)

$(call start-emulator-library,libsim_access_rules_proto)

LOCAL_CFLAGS := $(EMULATOR_COMMON_CFLAGS)

LOCAL_C_INCLUDES := \
    $(PROTOBUF_INCLUDES) \

LOCAL_PROTO_SOURCES := \
    $(SIM_ACCESS_RULES_DIR)/sim_access_rules.proto \

$(call end-emulator-library)

SIM_ACCESS_RULES_PROTO_STATIC_LIBRARIES := \
    libsim_access_rules_proto \
    $(PROTOBUF_STATIC_LIBRARIES)