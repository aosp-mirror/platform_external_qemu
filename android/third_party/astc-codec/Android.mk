###  astc-codec library, used for ASTC texture decompression in software.
###  The sources are located under $ANDROID/external/astc-codec.

EMULATOR_ASTC_CODEC_SOURCES_DIR ?= $(LOCAL_PATH)/../astc-codec
ifeq (,$(strip $(wildcard $(EMULATOR_ASTC_CODEC_SOURCES_DIR))))
    $(error Cannot find astc-codec sources directory: $(EMULATOR_ASTC_CODEC_SOURCES_DIR))
endif

EMULATOR_ASTC_CODEC_INCLUDES := $(EMULATOR_ASTC_CODEC_SOURCES_DIR)/include
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
ifeq (windows,$(BUILD_TARGET_OS))
    # MinGW has some additional warnings that are safe to ignore:
    #  - maybe-uninitialized in intermediate_astc_block.cc, for a std::array
    #    that is safely uninitialzed by a helper function.
    #  - unused-variable in release builds due to asserts. clang doesn't have
    #    this warning enabled, so it's safe to disable here.
    LOCAL_CFLAGS += -Wno-error=unused-variable -Wno-error=maybe-uninitialized
endif
LOCAL_C_INCLUDES += $(EMULATOR_ASTC_CODEC_SOURCES_DIR)
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := $(EMULATOR_ASTC_CODEC_SOURCES)
$(call end-emulator-library)


$(call start-emulator-program, emulator$(BUILD_TARGET_SUFFIX)_astc_unittests)
$(call gen-hw-config-defs)

LOCAL_C_INCLUDES += \
    $(EMULATOR_ASTC_CODEC_INCLUDES) \
    $(EMULATOR_GTEST_INCLUDES) \

LOCAL_SRC_FILES := \
    src/base/test/bit_stream_test.cpp \
    src/base/test/bottom_n_test.cpp \
    src/base/test/math_utils_test.cpp \
    src/base/test/optional_test.cpp \
    src/base/test/string_utils_test.cpp \
    src/base/test/type_traits_test.cpp \
    src/base/test/uint128_test.cpp \
    src/decoder/test/partition_test.cc \
    src/decoder/test/physical_astc_block_test.cc \
    src/decoder/test/integer_sequence_codec_test.cc \
    src/decoder/test/intermediate_astc_block_test.cc \
    src/decoder/test/quantization_test.cc \
    src/decoder/test/weight_infill_test.cc \
    src/decoder/test/endpoint_codec_test.cc \
    src/decoder/test/logical_astc_block_test.cc \
    src/decoder/test/codec_test.cc \
    src/decoder/test/footprint_test.cc \

LOCAL_COPY_FILES := \
    $(subst $(LOCAL_PATH),,$(wildcard $(LOCAL_PATH)/src/decoder/testdata/*.astc)) \
    $(subst $(LOCAL_PATH),,$(wildcard $(LOCAL_PATH)/src/decoder/testdata/*.bmp)) \

LOCAL_STATIC_LIBRARIES += \
    emulator-astc-codec \
    $(EMULATOR_GTEST_STATIC_LIBRARIES) \
    $(ANDROID_EMU_STATIC_LIBRARIES)

LOCAL_LDLIBS += \
    $(ANDROID_EMU_LDLIBS) \

# Link against static libstdc++ on Linux and Windows since the unit-tests
# cannot pick up our custom versions of the library from
# $(BUILD_OBJS_DIR)/lib[64]/
$(call local-link-static-c++lib)

$(call end-emulator-program)

LOCAL_PATH := $(old_LOCAL_PATH)

