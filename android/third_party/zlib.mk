ifeq ($(BUILD_TARGET_OS),windows_msvc)
    $(call define-emulator-prebuilt-library,\
        emulator-zlib,\
        $(ZLIB_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/lib/zlibstatic.lib)
else
    $(call define-emulator-prebuilt-library,\
        emulator-zlib,\
        $(ZLIB_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/lib/libz.a)
endif

ZLIB_INCLUDES := $(ZLIB_PREBUILTS_DIR)/$(BUILD_TARGET_OS)-x86_64/include
