# Build rules for prebuilt libvpx static library

LIBVPX_TOP_DIR := $(LIBVPX_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)

ifeq ($(BUILD_TARGET_OS),windows_msvc)
    $(call define-emulator-prebuilt-library, \
        emulator-libvpx, \
        $(LIBVPX_TOP_DIR)/lib/vpxmt.lib)
else
    $(call define-emulator-prebuilt-library, \
        emulator-libvpx, \
        $(LIBVPX_TOP_DIR)/lib/libvpx.a)
endif


LIBVPX_INCLUDES := $(LIBVPX_TOP_DIR)/include
LIBVPX_STATIC_LIBRARIES := emulator-libvpx
