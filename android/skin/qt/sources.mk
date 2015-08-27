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
ANDROID_SKIN_LDLIBS += $(LIBXML2_LDLIBS)
ANDROID_SKIN_LDLIBS_64 += $(LIBXML2_LDLIBS_64)

ANDROID_SKIN_SOURCES += \
    android/gps/GpxParser.cpp \
    android/gps/KmlParser.cpp \
    android/skin/qt/emulator-qt-window.cpp \
    android/skin/qt/tool-window.cpp \

ANDROID_SKIN_QT_MOC_SRC_FILES := \
    android/skin/qt/emulator-qt-window.h \
    android/skin/qt/tool-window.h \

ANDROID_SKIN_QT_RESOURCES := \
    android/skin/qt/resources.qrc \

ANDROID_SKIN_QT_UI_SRC_FILES := \
    android/skin/qt/extended.ui \
    android/skin/qt/tools.ui \
