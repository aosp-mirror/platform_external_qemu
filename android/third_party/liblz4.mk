ifeq ($(BUILD_TARGET_OS),windows_msvc)
    $(call define-emulator-prebuilt-library,\
        emulator-lz4,\
        $(LZ4_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/lib/liblz4_static.lib)
else
    $(call define-emulator-prebuilt-library,\
        emulator-lz4,\
        $(LZ4_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/lib/liblz4.a)
endif

LZ4_INCLUDES := $(LZ4_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/include
