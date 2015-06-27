# Definitions related to the Qt-based UI backend.

ANDROID_SKIN_SOURCES += \
    android/skin/qt/event-qt.cpp \
    android/skin/qt/surface-qt.cpp \
    android/skin/qt/winsys-qt.cpp \

ifeq (darwin,$(HOST_OS))
ANDROID_SKIN_SOURCES += \
    android/skin/qt/mac-native-window.mm
endif

ANDROID_SKIN_SOURCES += \
    android/skin/qt/emulator-window.cpp \
    android/skin/qt/tool-window.cpp \

ANDROID_SKIN_QT_MOC_SRC_FILES := \
    android/skin/qt/emulator-window.h \
    android/skin/qt/tool-window.h \

ANDROID_SKIN_QT_RESOURCES := \
    android/skin/qt/resources.qrc \
