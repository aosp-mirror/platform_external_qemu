# Build rules for prebuilt libvpx static library

LIBVPX_TOP_DIR := $(LIBVPX_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

$(call define-emulator-prebuilt-library, \
    emulator-libvpx, \
    $(LIBVPX_TOP_DIR)/lib/libvpx.a)


LIBVPX_INCLUDES := $(LIBVPX_TOP_DIR)/include
LIBVPX_STATIC_LIBRARIES := emulator-libvpx
