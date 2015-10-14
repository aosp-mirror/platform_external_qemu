$(call define-emulator-prebuilt-library,\
    emulator-libpng,\
    $(LIBPNG_PREBUILTS_DIR)/$(HOST_OS)-x86/lib/libpng.a)

$(call define-emulator64-prebuilt-library,\
    emulator64-libpng,\
    $(LIBPNG_PREBUILTS_DIR)/$(HOST_OS)-x86_64/lib/libpng.a)

LIBPNG_CFLAGS := -I$(LIBPNG_PREBUILTS_DIR)/$(HOST_OS)-x86_64/include

