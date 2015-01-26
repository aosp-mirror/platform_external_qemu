#-------------------------------------------------
#
# Project created by QtCreator 2014-12-03T16:06:48
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += $$(QMAKE_ARCH)

64bit {
    TARGET = qemu-qt-64
    QMAKE_CXXFLAGS += -m64 -O0
} else {
    TARGET = qemu-qt-32
    QMAKE_CXXFLAGS += -m32 -O0
}
build_pass:CONFIG(debug, debug|release) {
     unix: TARGET = $$join(TARGET,,,_debug)
     else: TARGET = $$join(TARGET,,,d)
 }
QMAKE_CC  = /usr/local/google/home/sbarta/src/studio-master-dev/prebuilts/gcc/linux-x86/host/x86_64-w64-mingw32-4.8/bin/x86_64-w64-mingw32-gcc
QMAKE_CXX = /usr/local/google/home/sbarta/src/studio-master-dev/prebuilts/gcc/linux-x86/host/x86_64-w64-mingw32-4.8/bin/x86_64-w64-mingw32-g++
QMAKE_LIB = /usr/local/google/home/sbarta/src/studio-master-dev/prebuilts/gcc/linux-x86/host/x86_64-w64-mingw32-4.8/bin/x86_64-w64-mingw32-ar -ru
TEMPLATE = lib
CONFIG += staticlib
CONFIG += debug_and_release
CONFIG += build_all
CONFIG += console
DESTDIR = ../../../objs/libs
INCLUDEPATH += ../../..
MOC_DIR = ../../../objs/intermediates/qt
OBJECTS_DIR = ../../../objs/intermediates/qt

SOURCES += \
        event-qt.cpp \
        surface-qt.cpp \
        winsys-qt.cpp \
        qt-context.cpp \
        emulator-window.cpp

HEADERS  += \
    ../event.h \
    ../surface.h \
    ../winsys.h \
    qt-context.h \
    emulator-window.h

FORMS += \
    emulator-window.ui

RESOURCES += \
    resources.qrc
