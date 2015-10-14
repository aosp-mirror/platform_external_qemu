$(call define-emulator-prebuilt-library,\
    emulator-zlib,\
    $(ZLIB_PREBUILTS_DIR)/$(HOST_OS)-x86/lib/libz.a)

$(call define-emulator64-prebuilt-library,\
    emulator64-zlib,\
    $(ZLIB_PREBUILTS_DIR)/$(HOST_OS)-x86_64/lib/libz.a)

ZLIB_INCLUDES := $(ZLIB_PREBUILTS_DIR)/$(HOST_OS)-x86_64/include
