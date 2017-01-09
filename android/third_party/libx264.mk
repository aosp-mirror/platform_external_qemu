# Build rules for prebuilt libx264 static library

LIBX264_TOP_DIR := $(X264_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

$(call define-emulator-prebuilt-library, \
    emulator-libx264, \
    $(LIBX264_TOP_DIR)/lib/libx264.a)


LIBX264_INCLUDES := $(LIBX264_TOP_DIR)/include
LIBX264_STATIC_LIBRARIES := emulator-libx264
