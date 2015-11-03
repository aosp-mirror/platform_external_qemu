# Definitions related to the Qt-based UI backend.

ANDROID_SKIN_SOURCES += \
    android/skin/qt/event-qt.cpp \
    android/skin/qt/surface-qt.cpp \
    android/skin/qt/winsys-qt.cpp \

ifeq (darwin,$(HOST_OS))
ANDROID_SKIN_SOURCES += \
    android/skin/qt/mac-native-window.mm
endif

ANDROID_SKIN_CFLAGS += -I$(LIBXML2_INCLUDES)
ANDROID_SKIN_STATIC_LIBRARIES += $(LIBXML2_STATIC_LIBRARIES)

ANDROID_SKIN_SOURCES += \
    android/gps/GpxParser.cpp \
    android/gps/KmlParser.cpp \
    android/skin/qt/editable-slider-widget.cpp \
    android/skin/qt/emulator-qt-window.cpp \
    android/skin/qt/ext-battery.cpp \
    android/skin/qt/ext-cellular.cpp \
    android/skin/qt/ext-dpad.cpp \
    android/skin/qt/ext-finger.cpp \
    android/skin/qt/ext-keyboard-shortcuts.cpp \
    android/skin/qt/ext-location.cpp \
    android/skin/qt/ext-settings.cpp \
    android/skin/qt/ext-sms.cpp \
    android/skin/qt/ext-telephony.cpp \
    android/skin/qt/ext-virtsensors.cpp \
    android/skin/qt/extended-window.cpp \
    android/skin/qt/qt-ui-commands.cpp \
    android/skin/qt/tool-window.cpp \

ANDROID_SKIN_QT_MOC_SRC_FILES := \
    android/skin/qt/editable-slider-widget.h \
    android/skin/qt/emulator-qt-window.h \
    android/skin/qt/extended-window.h \
    android/skin/qt/tool-window.h \

ANDROID_SKIN_QT_RESOURCES := \
    android/skin/qt/resources.qrc \

ANDROID_SKIN_QT_UI_SRC_FILES := \
    android/skin/qt/extended.ui \
    android/skin/qt/tools.ui \
