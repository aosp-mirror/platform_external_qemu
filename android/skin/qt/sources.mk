# Definitions related to the Qt-based UI backend.

ANDROID_SKIN_SOURCES += \
    android/skin/qt/event-qt.cpp \
    android/skin/qt/surface-qt.cpp \
    android/skin/qt/winsys-qt.cpp \

ifeq (darwin,$(BUILD_TARGET_OS))
ANDROID_SKIN_SOURCES += \
    android/skin/qt/mac-native-window.mm
endif

ANDROID_SKIN_CFLAGS += -I$(LIBXML2_INCLUDES)
ANDROID_SKIN_STATIC_LIBRARIES += $(LIBXML2_STATIC_LIBRARIES)

ANDROID_SKIN_SOURCES += \
    android/skin/qt/angle-input-widget.cpp \
    android/skin/qt/editable-slider-widget.cpp \
    android/skin/qt/emulator-qt-window.cpp \
    android/skin/qt/error-dialog.cpp \
    android/skin/qt/extended-pages/common.cpp \
    android/skin/qt/extended-pages/battery-page.cpp \
    android/skin/qt/extended-pages/cellular-page.cpp \
    android/skin/qt/extended-pages/dpad-page.cpp \
    android/skin/qt/extended-pages/finger-page.cpp \
    android/skin/qt/extended-pages/help-page.cpp \
    android/skin/qt/extended-pages/telephony-page.cpp \
    android/skin/qt/ext-location.cpp \
    android/skin/qt/ext-settings.cpp \
    android/skin/qt/ext-virtsensors.cpp \
    android/skin/qt/extended-window.cpp \
    android/skin/qt/QtLooper.cpp \
    android/skin/qt/qt-ui-commands.cpp \
    android/skin/qt/tool-window.cpp \

ANDROID_SKIN_QT_MOC_SRC_FILES := \
    android/skin/qt/angle-input-widget.h \
    android/skin/qt/editable-slider-widget.h \
    android/skin/qt/emulator-qt-window.h \
    android/skin/qt/extended-pages/battery-page.h \
    android/skin/qt/extended-pages/cellular-page.h \
    android/skin/qt/extended-pages/dpad-page.h \
    android/skin/qt/extended-pages/finger-page.h \
    android/skin/qt/extended-pages/help-page.h \
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
    android/skin/qt/extended-pages/telephony-page.ui \
    android/skin/qt/tools.ui \
