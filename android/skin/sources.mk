# Definitions related to the emulator's UI implementation.
# Note that there are two possible backends, based on the definitions
# of EMULATOR_USE_SDL2 or EMULATOR_USE_QT.

ANDROID_SKIN_LDLIBS :=
ANDROID_SKIN_LDLIBS_64 :=
ANDROID_SKIN_CFLAGS :=

ANDROID_SKIN_QT_MOC_SRC_FILES :=
ANDROID_SKIN_QT_RESOURCES :=

ANDROID_SKIN_SOURCES := \
    android/skin/charmap.c \
    android/skin/rect.c \
    android/skin/region.c \
    android/skin/image.c \
    android/skin/trackball.c \
    android/skin/keyboard.c \
    android/skin/keycode.c \
    android/skin/keycode-buffer.c \
    android/skin/keyset.c \
    android/skin/file.c \
    android/skin/window.c \
    android/skin/resource.c \
    android/skin/scaler.c \
    android/skin/ui.c \

ifdef EMULATOR_USE_SDL2
ANDROID_SKIN_SOURCES += \
    android/skin/event-sdl2.c \
    android/skin/surface-sdl2.c \
    android/skin/winsys-sdl2.c \

endif  # EMULATOR_USE_SDL2

ifdef EMULATOR_USE_QT
include $(LOCAL_PATH)/android/skin/qt/sources.mk
endif  # EMULATOR_USE_QT
