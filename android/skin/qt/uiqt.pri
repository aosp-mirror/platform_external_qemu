#-------------------------------------------------
#
# Project created by QtCreator 2014-12-03T16:06:48
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += $${QMAKE_ARCH}
m64 {
    TARGET = uiqt64
    QMAKE_CXXFLAGS += -m64 -O0
    DESTDIR = $${SOURCE_BASE}/objs/libs64
} else {
    TARGET = uiqt
    QMAKE_CXXFLAGS += -m32 -O0
    DESTDIR = $${SOURCE_BASE}/objs/libs
}
build_pass:CONFIG(debug, debug|release) {
     unix: TARGET = $$join(TARGET,,,_debug)
     else: TARGET = $$join(TARGET,,,d)
}

QMAKE_CC  = $${SOURCE_BASE}/prebuilts/clang/darwin-x86/host/3.5/bin/clang
QMAKE_CXX = $${SOURCE_BASE}/prebuilts/clang/darwin-x86/host/3.5/bin/clang
QMAKE_LIB = $${SOURCE_BASE}/prebuilts/clang/darwin-x86/host/3.5/bin/clang

TEMPLATE = lib
CONFIG += staticlib
CONFIG += debug_and_release
CONFIG += build_all
CONFIG += console
INCLUDEPATH += $${SOURCE_BASE}
MOC_DIR = $${SOURCE_BASE}/objs/intermediates/qt/$${QMAKE_ARCH}
OBJECTS_DIR = $${SOURCE_BASE}/objs/intermediates/qt/$${QMAKE_ARCH}

SOURCES += \
    $${SOURCE_BASE}/android/skin/qt/event-qt.cpp \
    $${SOURCE_BASE}/android/skin/qt/surface-qt.cpp \
    $${SOURCE_BASE}/android/skin/qt/winsys-qt.cpp \
    $${SOURCE_BASE}/android/skin/qt/qt-context.cpp \
    $${SOURCE_BASE}/android/skin/qt/emulator-window.cpp

HEADERS  += \
    $${SOURCE_BASE}/android/skin/event.h \
    $${SOURCE_BASE}/android/skin/surface.h \
    $${SOURCE_BASE}/android/skin/winsys.h \
    $${SOURCE_BASE}/android/skin/qt/qt-context.h \
    $${SOURCE_BASE}/android/skin/qt/emulator-window.h

FORMS += \
    $${SOURCE_BASE}/android/skin/qt/emulator-window.ui

RESOURCES += \
    $${SOURCE_BASE}/android/skin/qt/resources.qrc
