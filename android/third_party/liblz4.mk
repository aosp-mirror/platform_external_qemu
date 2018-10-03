$(call define-emulator-prebuilt-library,\
    emulator-lz4,\
    $(LZ4_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/lib/liblz4$(BUILD_TARGET_STATIC_LIBEXT))

LZ4_INCLUDES := $(LZ4_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/include
