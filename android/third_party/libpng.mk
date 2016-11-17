$(call define-emulator-prebuilt-library,\
    emulator-libpng,\
    $(LIBPNG_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/lib/libpng.a)

LIBPNG_INCLUDES := $(LIBPNG_PREBUILTS_DIR)/$(BUILD_TARGET_OS)-x86_64/include
