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
    macx {
        QMAKE_CXXFLAGS += -arch x86_64
        QMAKE_OBJECTIVE_CFLAGS += -arch x86_64
    }
} else {
    TARGET = uiqt
    QMAKE_CXXFLAGS += -m32 -O0
    DESTDIR = $${SOURCE_BASE}/objs/libs
    macx {
        QMAKE_CXXFLAGS += -arch i386
        QMAKE_OBJECTIVE_CFLAGS += -arch i386
    }
}
build_pass:CONFIG(debug, debug|release) {
     unix: TARGET = $$join(TARGET,,,_debug)
     else: TARGET = $$join(TARGET,,,d)
}

macx {
    QMAKE_CXX = $${SOURCE_BASE}/prebuilts/clang/darwin-x86/host/3.5/bin/clang
    QMAKE_LIB = $${SOURCE_BASE}/prebuilts/clang/darwin-x86/host/3.5/bin/clang
}
linux {
    QMAKE_CXX = $${SOURCE_BASE}/../../prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.11-4.8/bin/x86_64-linux-g++
    QMAKE_LIB = $${SOURCE_BASE}/../../prebuilts/gcc/linux-x86/host/x86_64-linux-glibc2.11-4.8/bin/x86_64-linux-ar
}

TEMPLATE = lib
CONFIG += staticlib
CONFIG += debug_and_release
CONFIG += build_all
CONFIG += console
INCLUDEPATH += $${SOURCE_BASE}
INCLUDEPATH += /System/Library/Frameworks/Cocoa.framework/Versions/A/Headers
MOC_DIR = $${SOURCE_BASE}/objs/intermediates/qt/$${QMAKE_ARCH}
OBJECTS_DIR = $${SOURCE_BASE}/objs/intermediates/qt/$${QMAKE_ARCH}

SOURCES += \
    $${SOURCE_BASE}/android/skin/qt/event-qt.cpp \
    $${SOURCE_BASE}/android/skin/qt/surface-qt.cpp \
    $${SOURCE_BASE}/android/skin/qt/winsys-qt.cpp \
    $${SOURCE_BASE}/android/skin/qt/qt-context.cpp \
    $${SOURCE_BASE}/android/skin/qt/emulator-window.cpp

macx {
OBJECTIVE_SOURCES += \
    $${SOURCE_BASE}/android/skin/qt/native-mac-window.mm
}

HEADERS  += \
    $${SOURCE_BASE}/android/skin/event.h \
    $${SOURCE_BASE}/android/skin/surface.h \
    $${SOURCE_BASE}/android/skin/winsys.h \
    $${SOURCE_BASE}/android/skin/qt/qt-context.h \
    $${SOURCE_BASE}/android/skin/qt/emulator-window.h 

macx {
    $${SOURCE_BASE}/android/skin/qt/native-mac-window.h
}

FORMS += \
    $${SOURCE_BASE}/android/skin/qt/emulator-window.ui

RESOURCES += \
    $${SOURCE_BASE}/android/skin/qt/resources.qrc
