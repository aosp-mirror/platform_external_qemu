$(call define-emulator-prebuilt-library,\
    emulator-lz4,\
    $(LZ4_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/lib/liblz4.a)

LZ4_INCLUDES := $(LZ4_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/include
