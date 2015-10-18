$(call define-emulator-prebuilt-library,\
    emulator_libSDL2,\
    $(SDL2_PREBUILTS_DIR)/$(HOST_OS)-$(HOST_ARCH)/lib/libSDL2.a)

$(call define-emulator-prebuilt-library,\
    emulator_libSDL2main,\
    $(SDL2_PREBUILTS_DIR)/$(HOST_OS)-$(HOST_ARCH)/lib/libSDL2main.a)

SDL2_INCLUDES := $(SDL2_PREBUILTS_DIR)/$(HOST_OS)-x86_64/include
SDL2_CFLAGS :=

SDL2_LDLIBS_linux := -lm -ldl -lpthread -lrt
SDL2_LDLIBS_windows := -luser32 -lgdi32 -lwinmm -lvfw32 -limm32 -lversion -lole32 -loleaut32
SDL2_LDLIBS_darwin := -Wl,-framework,OpenGL,-framework,Cocoa,-framework,ApplicationServices,-framework,Carbon,-framework,IOKit

SDL2_LDLIBS := $(SDL2_LDLIBS_$(HOST_OS))
SDL2_STATIC_LIBRARIES := emulator_libSDL2 emulator_libSDL2main

