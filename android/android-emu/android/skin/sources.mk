# Definitions related to the emulator's UI implementation.

ANDROID_SKIN_LDLIBS :=
ANDROID_SKIN_CFLAGS :=
ANDROID_SKIN_INCLUDES :=
ANDROID_SKIN_STATIC_LIBRARIES :=

ANDROID_SKIN_QT_MOC_SRC_FILES :=
ANDROID_SKIN_QT_RESOURCES :=
# DEPRECATED! MODIFY THE CMAKELISTS.TXT instead.
#
# enable MMX code for our skin scaler
ANDROID_SKIN_CFLAGS += -DUSE_MMX=1 -mmmx

include $(_ANDROID_EMU_ROOT)/android/skin/qt/sources.mk
