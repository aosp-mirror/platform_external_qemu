$(call define-emulator-prebuilt-library,\
    emulator-sdl2,\
    $(LIBSDL2_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/lib/libSDL2.a)

SDL2_INCLUDES := $(LIBSDL2_PREBUILTS_DIR)/$(BUILD_TARGET_OS)/include
