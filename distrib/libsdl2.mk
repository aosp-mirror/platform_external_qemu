$(call define-emulator-prebuilt-library,\
    emulator_libSDL2,\
    $(SDL2_PREBUILTS_DIR)/$(HOST_OS)-x86/lib/libSDL2.a)

$(call define-emulator64-prebuilt-library,\
    emulator_lib64SDL2,\
    $(SDL2_PREBUILTS_DIR)/$(HOST_OS)-x86_64/lib/libSDL2.a)

$(call define-emulator-prebuilt-library,\
    emulator_libSDL2main,\
    $(SDL2_PREBUILTS_DIR)/$(HOST_OS)-x86/lib/libSDL2main.a)

$(call define-emulator64-prebuilt-library,\
    emulator_lib64SDL2main,\
    $(SDL2_PREBUILTS_DIR)/$(HOST_OS)-x86_64/lib/libSDL2main.a)

SDL2_INCLUDES := $(SDL2_PREBUILTS_DIR)/$(HOST_OS)-x86_64/include
SDL2_CFLAGS :=

SDL2_LDLIBS_linux := -lm -ldl -lpthread -lrt
SDL2_LDLIBS_windows := -luser32 -lgdi32 -lwinmm -lvfw32 -limm32 -lversion -lole32 -loleaut32
SDL2_LDLIBS_darwin := -Wl,-framework,OpenGL,-framework,Cocoa,-framework,ApplicationServices,-framework,Carbon,-framework,IOKit

SDL2_LDLIBS := $(SDL2_LDLIBS_$(HOST_OS))
SDL2_LDLIBS_64 := $(SDL2_LDLIBS_$(HOST_OS))

SDL2_STATIC_LIBRARIES := emulator_libSDL2 emulator_libSDL2main
SDL2_STATIC_LIBRARIES_64 := emulator_lib64SDL2 emulator_lib64SDL2main

