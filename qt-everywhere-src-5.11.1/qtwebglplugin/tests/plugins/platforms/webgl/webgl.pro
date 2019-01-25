CONFIG += testcase

QT += \
    testlib \
    quick \
    websockets

TARGET = tst_webgl

HEADERS += \
    parameters.h

SOURCES += \
    tst_webgl.cpp

TESTDATA = *.qml

OTHER_FILES += \
    basic_scene.qml
