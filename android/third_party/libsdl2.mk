ifeq ($(BUILD_TARGET_OS),windows_msvc)
    # TODO(joshuaduong): grab/compile a static version of SDL2
    $(call define-emulator-prebuilt-library,\
        emulator-sdl2,\
        $(LIBSDL2_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/lib/SDL2.lib)
else
    $(call define-emulator-prebuilt-library,\
        emulator-sdl2,\
        $(LIBSDL2_PREBUILTS_DIR)/$(BUILD_TARGET_TAG)/lib/libSDL2.a)
endif

SDL2_INCLUDES := $(LIBSDL2_PREBUILTS_DIR)/$(BUILD_TARGET_OS)/include
