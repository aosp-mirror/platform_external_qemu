$(call define-emulator-prebuilt-library,\
    emulator-libpng,\
    $(LIBPNG_PREBUILTS_DIR)/$(HOST_OS)-$(HOST_ARCH)/lib/libpng.a)

LIBPNG_CFLAGS := -I$(LIBPNG_PREBUILTS_DIR)/$(HOST_OS)-x86_64/include

