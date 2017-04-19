list(APPEND SOURCES
        QtLooper.cpp
        accelerometer-3d-widget.cpp
        angle-input-widget.cpp
        editable-slider-widget.cpp
        emulator-container.cpp
        emulator-overlay.cpp
        emulator-qt-no-window.cpp
        emulator-qt-window.cpp
        error-dialog.cpp
        event-capturer.cpp
        event-qt.cpp
        event-serializer.cpp
        event-subscriber.cpp
        extended-pages/battery-page.cpp
        extended-pages/bug-report-window.cpp
        extended-pages/car-data-emulation/car-sensor-data.cpp
        extended-pages/car-data-page.cpp
        extended-pages/cellular-page.cpp
        extended-pages/common.cpp
        extended-pages/dpad-page.cpp
        extended-pages/finger-page.cpp
        extended-pages/google-play-page.cpp
        extended-pages/help-page.cpp
        extended-pages/location-page.cpp
        extended-pages/microphone-page.cpp
        extended-pages/record-screen-page.cpp
        extended-pages/rotary-input-dial.cpp
        extended-pages/rotary-input-page.cpp
        extended-pages/settings-page-proxy.cpp
        extended-pages/settings-page.cpp
        extended-pages/telephony-page.cpp
        extended-pages/virtual-sensors-page.cpp
        extended-window.cpp
        gl-canvas.cpp
        gl-common.cpp
        gl-texture-draw.cpp
        gl-widget.cpp
        init-qt.cpp
        qt-ui-commands.cpp
        size-tweaker.cpp
        stylesheet.cpp
        surface-qt.cpp
        tool-window.cpp
        ui-event-recorder.cpp
        user-actions-counter.cpp
        wavefront-obj-parser.cpp
        #windows-native-window.cpp
        winsys-qt.cpp
        )

include_directories(
        ${CONFIG_HOST_INCLUDE_DIRS}
        ${ANDROID_EMUGL}/host/include
        ${ANDROID_EMUGL}/shared
        ${Protobuf_INCLUDE_DIR}
        ${QT5_INCLUDE_DIRS}
        ${CMAKE_CURRENT_BINARY_DIR}
        )
add_qt_support()
add_library(skin_qt ${SOURCES})

if (APPLE)
    add_dependencies(skin_qt fix_uic)
endif ()

