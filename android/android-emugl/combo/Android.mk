LOCAL_PATH := $(call my-dir)

combo_C_INCLUDES := \
    $(LOCAL_PATH) \
    $(EMUGL_PATH)/host/include \
    $(EMUGL_PATH)/host/include/OpenGLESDispatch \
    $(EMUGL_PATH)/host/libs/libOpenglRender/standalone_common \
    $(EMUGL_PATH)/host/libs/libOpenglRender/standalone_common/angle-util \
	$(EMUGL_PATH)/guest \
	$(EMUGL_PATH)/guest/androidImpl \

combo_STATIC_LIBRARIES := \
    $(ANDROID_EMU_STATIC_LIBRARIES) \
    $(ANGLE_TRANSLATION_STATIC_LIBRARIES) \
    libOpenGLESDispatch \
    libGLSnapshot \
    libemugl_common \
    android-emu \
    android-emu-base \
    emulator-lz4 \

combo_LDLIBS := -lm

ifeq ($(BUILD_TARGET_OS),linux)
    combo_LDLIBS += -lX11 -lrt
endif

ifeq ($(BUILD_TARGET_OS),darwin)
    combo_LDLIBS += \
        -Wl,-framework,AppKit, \
        -Wl,-framework,CoreMedia, \
        -Wl,-framework,AudioUnit, \
        -Wl,-framework,AVFoundation, \
        -Wl,-framework,Cocoa, \
        -Wl,-framework,CoreAudio, \
        -Wl,-framework,CoreMedia, \
        -Wl,-framework,CoreVideo, \
        -Wl,-framework,ForceFeedback, \
        -Wl,-framework,IOKit, \
        -Wl,-framework,QTKit, \
        -Wl,-framework,VideoDecodeAcceleration, \
        -Wl,-framework,VideoToolbox
endif

ifeq ($(BUILD_TARGET_OS),windows)
    combo_LDLIBS += -lgdi32
endif

combo_LDFLAGS :=

ifeq ($(BUILD_TARGET_OS),darwin)
    combo_LDFLAGS += -Wl,-headerpad_max_install_names
endif

$(call emugl-begin-executable,libEGLGoldfish_unittest)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)AndroidGraphics)

LOCAL_C_INCLUDES += $(combo_C_INCLUDES)
LOCAL_STATIC_LIBRARIES += $(combo_STATIC_LIBRARIES)
LOCAL_SRC_FILES := \
	tests/libEGLGoldfish_unittest.cpp \

LOCAL_LDFLAGS += $(combo_LDFLAGS)
LOCAL_LDLIBS += $(combo_LDLIBS)

$(call emugl-end-module)

$(call emugl-begin-shared-library,libEGL_aemu)
$(call emugl-import,lib$(BUILD_TARGET_SUFFIX)AndroidGraphics)

LOCAL_C_INCLUDES += $(combo_C_INCLUDES)
LOCAL_STATIC_LIBRARIES += $(combo_STATIC_LIBRARIES)
LOCAL_SRC_FILES := \
	egl.cpp \

LOCAL_LDFLAGS += $(combo_LDFLAGS)
LOCAL_LDLIBS += $(combo_LDLIBS)

$(call emugl-end-module)
