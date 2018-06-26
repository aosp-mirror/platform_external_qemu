###  astc-codec library, used for ASTC texture decompression in software.
###  The sources are located under $ANDROID/external/astc-codec.

EMULATOR_ASTC_CODEC_SOURCES_DIR ?= $(LOCAL_PATH)/../astc-codec
ifeq (,$(strip $(wildcard $(EMULATOR_ASTC_CODEC_SOURCES_DIR))))
    $(error Cannot find astc-codec sources directory: $(EMULATOR_ASTC_CODEC_SOURCES_DIR))
endif

EMULATOR_ASTC_CODEC_INCLUDES := $(EMULATOR_ASTC_CODEC_SOURCES_DIR)\include
EMULATOR_ASTC_CODEC_SOURCES := \
    src/decoder/astc_file.cc \
    src/decoder/codec.cc \
    src/decoder/endpoint_codec.cc \
    src/decoder/footprint.cc \
    src/decoder/integer_sequence_codec.cc \
    src/decoder/intermediate_astc_block.cc \
    src/decoder/logical_astc_block.cc \
    src/decoder/partition.cc \
    src/decoder/physical_astc_block.cc \
    src/decoder/quantization.cc \
    src/decoder/weight_infill.cc \

old_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(EMULATOR_ASTC_CODEC_SOURCES_DIR)

$(call start-emulator-library, emulator-astc-codec)
LOCAL_C_INCLUDES += $(EMULATOR_ASTC_CODEC_SOURCES_DIR)
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := $(EMULATOR_ASTC_CODEC_SOURCES)
$(call end-emulator-library)

LOCAL_PATH := $(old_LOCAL_PATH)

