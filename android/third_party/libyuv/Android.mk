###########################################################
###########################################################
###
###  libyuv library.
###
###  libyuv is used by the host for optimized colorspace
###  conversion on the host. The sources are located under
###  $ANDROID/external/libyuv, but because we need to build
###  both 32-bit and 64-bit host libraries, don't use the
###  .mk file there, define a module here instead.

EMULATOR_LIBYUV_SOURCES_DIR ?= $(LOCAL_PATH)/../libyuv/files
EMULATOR_LIBYUV_SOURCES_DIR := $(EMULATOR_LIBYUV_SOURCES_DIR)
ifeq (,$(strip $(wildcard $(EMULATOR_LIBYUV_SOURCES_DIR))))
    $(error Cannot find libyuv sources directory: $(EMULATOR_LIBYUV_SOURCES_DIR))
endif

EMULATOR_LIBYUV_INCLUDES := $(EMULATOR_LIBYUV_SOURCES_DIR)/include
EMULATOR_LIBYUV_SOURCES := \
    source/compare.cc           \
    source/compare_common.cc    \
    source/compare_gcc.cc       \
    source/compare_neon64.cc    \
    source/compare_neon.cc      \
    source/convert_argb.cc      \
    source/convert.cc           \
    source/convert_from_argb.cc \
    source/convert_from.cc      \
    source/convert_jpeg.cc      \
    source/convert_to_argb.cc   \
    source/convert_to_i420.cc   \
    source/cpu_id.cc            \
    source/mjpeg_decoder.cc     \
    source/mjpeg_validate.cc    \
    source/planar_functions.cc  \
    source/rotate_any.cc        \
    source/rotate_argb.cc       \
    source/rotate.cc            \
    source/rotate_common.cc     \
    source/rotate_gcc.cc        \
    source/rotate_dspr2.cc      \
    source/rotate_neon64.cc     \
    source/rotate_neon.cc       \
    source/rotate_win.cc        \
    source/row_any.cc           \
    source/row_common.cc        \
    source/row_gcc.cc           \
    source/row_dspr2.cc         \
    source/row_neon64.cc        \
    source/row_neon.cc          \
    source/row_win.cc           \
    source/scale_any.cc         \
    source/scale_argb.cc        \
    source/scale.cc             \
    source/scale_common.cc      \
    source/scale_gcc.cc         \
    source/scale_dspr2.cc       \
    source/scale_neon64.cc      \
    source/scale_neon.cc        \
    source/scale_win.cc         \
    source/video_common.cc

old_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := $(EMULATOR_LIBYUV_SOURCES_DIR)

$(call start-emulator-library, emulator-libyuv)
LOCAL_C_INCLUDES += $(EMULATOR_LIBYUV_INCLUDES)
LOCAL_CPP_EXTENSION := .cc
ifeq ($(BUILD_TARGET_OS),windows)
    # Turn off libyuv assembly on Windows x86, libyuv assembly fails to compile
    # on that flavor.
    LOCAL_CFLAGS += -DLIBYUV_DISABLE_X86
endif
LOCAL_CFLAGS += -fno-strict-aliasing
LOCAL_SRC_FILES := $(EMULATOR_LIBYUV_SOURCES)
$(call end-emulator-library)

LOCAL_PATH := $(old_LOCAL_PATH)

