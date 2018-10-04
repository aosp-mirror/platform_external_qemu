$(call define-emulator-prebuilt-library,\
    emulator-sdl2,\
    $(LIBSDL2_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/lib/libSDL2$(BUILD_TARGET_STATIC_LIBEXT))

SDL2_INCLUDES := $(LIBSDL2_PREBUILTS_DIR)/$(BUILD_TARGET_OS)/include
