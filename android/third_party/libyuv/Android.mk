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
old_LOCAL_PATH := $(LOCAL_PATH)

LOCAL_PATH := android/third_party/libyuv
$(call start-cmake-project,emulator-libyuv)
PRODUCED_STATIC_LIBS := emulator-libyuv
$(call end-cmake-project)

LOCAL_PATH := $(old_LOCAL_PATH)

