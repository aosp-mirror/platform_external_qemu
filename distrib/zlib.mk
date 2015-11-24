$(call define-emulator-prebuilt-library,\
    emulator-zlib,\
    $(ZLIB_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/lib/libz.a)

ZLIB_INCLUDES := $(ZLIB_PREBUILTS_DIR)/$(BUILD_TARGET_OS)-x86_64/include
