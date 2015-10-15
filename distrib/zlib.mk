$(call define-emulator-prebuilt-library,\
    emulator-zlib,\
    $(ZLIB_PREBUILTS_DIR)/$(HOST_OS)-$(HOST_ARCH)/lib/libz.a)

ZLIB_INCLUDES := $(ZLIB_PREBUILTS_DIR)/$(HOST_OS)-x86_64/include
