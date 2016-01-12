# Definitions related to the Qt-based UI backend.

ANDROID_SKIN_SOURCES += \
    android/skin/qt/event-qt.cpp \
    android/skin/qt/surface-qt.cpp \
    android/skin/qt/winsys-qt.cpp \

ifeq (darwin,$(BUILD_TARGET_OS))
ANDROID_SKIN_SOURCES += \
    android/skin/qt/mac-native-window.mm
endif

include distrib/android-emugl/common.mk

OPENGLES_DISPATCH_INCLUDES := distrib/android-emugl/host/libs/libOpenGLESDispatch
OPENGLES_DISPATCH_LIB := libOpenGLESDispatch
EGL_INCLUDES := distrib/android-emugl/host/libs/Translator/include
GLES2_DEC_INTERMEDIATE_INCLUDES := $(call intermediates-dir-for,$(BUILD_TARGET_BITS),gles2) 
GLES2_DEC_INCLUDES := distrib/android-emugl/host/libs/GLESv2_dec
GL_CODEC_COMMON_INCLUDES := distrib/android-emugl/shared/OpenglCodecCommon
GL_TRANSLATOR_INCLUDES := distrib/android-emugl/host/libs/Translator/include

ANDROID_SKIN_CFLAGS += \
                       -I$(LIBXML2_INCLUDES) \
                       -I$(OPENGLES_DISPATCH_INCLUDES) \
                       -I$(EGL_INCLUDES) \
                       -I$(GLES2_DEC_INTERMEDIATE_INCLUDES) \
                       -I$(GLES2_DEC_INCLUDES) \
                       -I$(GL_CODEC_COMMON_INCLUDES) \
                       -I$(GL_TRANSLATOR_INCLUDES) \
                       -Idistrib/android-emugl/host/include/libOpenglRender

GL_LIBS := libemugl_common libGLESv2_dec

ANDROID_SKIN_STATIC_LIBRARIES += $(LIBXML2_STATIC_LIBRARIES) $(OPENGLES_DISPATCH_LIB) $(GL_LIBS)

ANDROID_SKIN_SOURCES += \
    $(call intermediates-dir-for,$(BUILD_TARGET_BITS),glwidget-gles2)/gles2_server_context.h \
    android/skin/qt/angle-input-widget.cpp \
    android/skin/qt/editable-slider-widget.cpp \
    android/skin/qt/emulator-qt-window.cpp \
    android/skin/qt/emulator-qt-no-window.cpp \
    android/skin/qt/error-dialog.cpp \
    android/skin/qt/gl-widget.cpp \
    android/skin/qt/extended-pages/common.cpp \
    android/skin/qt/extended-pages/battery-page.cpp \
    android/skin/qt/extended-pages/cellular-page.cpp \
    android/skin/qt/extended-pages/dpad-page.cpp \
    android/skin/qt/extended-pages/finger-page.cpp \
    android/skin/qt/extended-pages/help-page.cpp \
    android/skin/qt/extended-pages/settings-page.cpp \
    android/skin/qt/extended-pages/telephony-page.cpp \
    android/skin/qt/ext-location.cpp \
    android/skin/qt/ext-virtsensors.cpp \
    android/skin/qt/extended-window.cpp \
    android/skin/qt/QtLooper.cpp \
    android/skin/qt/qt-ui-commands.cpp \
    android/skin/qt/tool-window.cpp \

ANDROID_SKIN_QT_MOC_SRC_FILES := \
    android/skin/qt/angle-input-widget.h \
    android/skin/qt/editable-slider-widget.h \
    android/skin/qt/gl-widget.h \
    android/skin/qt/emulator-qt-window.h \
    android/skin/qt/emulator-qt-no-window.h \
    android/skin/qt/extended-pages/battery-page.h \
    android/skin/qt/extended-pages/cellular-page.h \
    android/skin/qt/extended-pages/dpad-page.h \
    android/skin/qt/extended-pages/finger-page.h \
    android/skin/qt/extended-pages/help-page.h \
    android/skin/qt/extended-pages/settings-page.h \
    android/skin/qt/extended-pages/telephony-page.h \
    android/skin/qt/extended-window.h \
    android/skin/qt/QtTimerImpl.h \
    android/skin/qt/tool-window.h \

ANDROID_SKIN_QT_RESOURCES := \
    android/skin/qt/resources.qrc \

ANDROID_SKIN_QT_UI_SRC_FILES := \
    android/skin/qt/extended.ui \
    android/skin/qt/extended-pages/battery-page.ui \
    android/skin/qt/extended-pages/cellular-page.ui \
    android/skin/qt/extended-pages/dpad-page.ui \
    android/skin/qt/extended-pages/finger-page.ui \
    android/skin/qt/extended-pages/help-page.ui \
    android/skin/qt/extended-pages/settings-page.ui \
    android/skin/qt/extended-pages/telephony-page.ui \
    android/skin/qt/tools.ui \
