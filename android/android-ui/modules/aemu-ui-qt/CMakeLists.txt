android_qt_lib(
  TARGET ext-page-camera
  SRC android/skin/qt/device-3d-widget.cpp
      android/skin/qt/extended-pages/camera-page.cpp
      android/skin/qt/extended-pages/camera-page.ui
      android/skin/qt/extended-pages/camera-virtualscene-subpage.cpp
      android/skin/qt/extended-pages/camera-virtualscene-subpage.ui
      android/skin/qt/extended-pages/virtual-sensors-page.cpp
      android/skin/qt/extended-pages/virtual-sensors-page.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Svg ui::gl-qt-bridge ui::widgets)

android_qt_lib(
  TARGET ext-page-car
  SRC android/skin/qt/car-cluster-window.cpp
      android/skin/qt/extended-pages/car-cluster-connector/car-cluster-connector.cpp
      android/skin/qt/extended-pages/car-data-emulation/car-property-utils.cpp
      android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.cpp
      android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.ui
      android/skin/qt/extended-pages/car-data-emulation/checkbox-dialog.cpp
      android/skin/qt/extended-pages/car-data-emulation/vhal-item.cpp
      android/skin/qt/extended-pages/car-data-emulation/vhal-table.cpp
      android/skin/qt/extended-pages/car-data-page.cpp
      android/skin/qt/extended-pages/car-data-page.ui
      android/skin/qt/extended-pages/car-rotary-page.cpp
      android/skin/qt/extended-pages/car-rotary-page.ui
      android/skin/qt/extended-pages/instr-cluster-render/car-cluster-widget.cpp
  DEPS android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::common
       ui::gl-qt-bridge
       ui::widgets)

android_qt_lib(
  TARGET ext-page-recording
  SRC android/skin/qt/extended-pages/record-and-playback-page.cpp
      android/skin/qt/extended-pages/record-and-playback-page.h
      android/skin/qt/extended-pages/record-and-playback-page.ui
      android/skin/qt/extended-pages/record-macro-edit-dialog.cpp
      android/skin/qt/extended-pages/record-macro-edit-dialog.h
      android/skin/qt/extended-pages/record-macro-edit-dialog.ui
      android/skin/qt/extended-pages/record-macro-page.cpp
      android/skin/qt/extended-pages/record-macro-page.h
      android/skin/qt/extended-pages/record-macro-page.ui
      android/skin/qt/extended-pages/record-macro-saved-item.cpp
      android/skin/qt/extended-pages/record-screen-page-tasks.h
      android/skin/qt/extended-pages/record-screen-page.cpp
      android/skin/qt/extended-pages/record-screen-page.h
      android/skin/qt/extended-pages/record-screen-page.ui
      android/skin/qt/extended-pages/record-settings-page.cpp
      android/skin/qt/extended-pages/record-settings-page.h
      android/skin/qt/extended-pages/record-settings-page.ui
      android/skin/qt/video-player/QtVideoPlayerNotifier.cpp
      android/skin/qt/video-player/VideoInfo.cpp
      android/skin/qt/video-player/VideoPlayerWidget.cpp
  DEPS aemu::recording
       android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::gl-qt-bridge
       ui::widgets)

android_qt_lib(
  TARGET ext-page-bugreport
  SRC android/skin/qt/extended-pages/bug-report-page.cpp
      android/skin/qt/extended-pages/bug-report-page.ui
  DEPS android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::common
       ui::gl-qt-bridge
       ui::widgets)

android_qt_lib(
  TARGET ext-page-battery
  SRC android/skin/qt/extended-pages/battery-page.cpp
      android/skin/qt/extended-pages/battery-page.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)

android_qt_lib(
  TARGET ext-page-cellular
  SRC android/skin/qt/extended-pages/cellular-page.cpp
      android/skin/qt/extended-pages/cellular-page.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)

android_qt_lib(
  TARGET ext-page-dpad
  SRC android/skin/qt/extended-pages/dpad-page.cpp
      android/skin/qt/extended-pages/dpad-page.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)

android_qt_lib(
  TARGET ext-page-finger
  SRC android/skin/qt/extended-pages/finger-page.cpp
      android/skin/qt/extended-pages/finger-page.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)

android_qt_lib(
  TARGET ext-page-google-play
  SRC android/skin/qt/extended-pages/google-play-page.cpp
      android/skin/qt/extended-pages/google-play-page.ui
  DEPS android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::common
       ui::gl-qt-bridge
       ui::widgets)

android_qt_lib(
  TARGET ext-page-help
  SRC android/skin/qt/extended-pages/help-page.cpp
      android/skin/qt/extended-pages/help-page.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)

android_qt_lib(
  TARGET ext-page-microphone
  SRC android/skin/qt/extended-pages/microphone-page.cpp
      android/skin/qt/extended-pages/microphone-page.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)

android_qt_lib(
  TARGET ext-page-display
  SRC android/skin/qt/extended-pages/multi-display-arrangement.cpp
      android/skin/qt/extended-pages/multi-display-item.cpp
      android/skin/qt/extended-pages/multi-display-page.cpp
      android/skin/qt/extended-pages/multi-display-page.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)

android_qt_lib(
  TARGET ext-page-rotary
  SRC android/skin/qt/extended-pages/rotary-input-dial.cpp
      android/skin/qt/extended-pages/rotary-input-page.cpp
      android/skin/qt/extended-pages/rotary-input-page.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Svg ui::gl-qt-bridge ui::widgets)

android_qt_lib(
  TARGET ext-page-settings
  SRC android/skin/qt/extended-pages/perfstats-page.cpp
      android/skin/qt/extended-pages/perfstats-page.ui
      android/skin/qt/extended-pages/settings-page-proxy.cpp
      android/skin/qt/extended-pages/settings-page.cpp
      android/skin/qt/extended-pages/settings-page.ui
      android/skin/qt/perf-stats-3d-widget.cpp
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)

android_qt_lib(
  TARGET ext-page-snapshot
  SRC android/skin/qt/extended-pages/snapshot-page.cpp
      android/skin/qt/extended-pages/snapshot-page.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)

android_qt_lib(
  TARGET ext-page-telephony
  SRC android/skin/qt/extended-pages/telephony-page.cpp
      android/skin/qt/extended-pages/telephony-page.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)

android_qt_lib(
  TARGET ext-page-tv-remote
  SRC android/skin/qt/extended-pages/tv-remote-page.cpp
      android/skin/qt/extended-pages/tv-remote-page.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)

android_qt_lib(
  TARGET ext-page-sensor-replay
  SRC android/skin/qt/extended-pages/car-data-emulation/car-property-utils.cpp
      android/skin/qt/extended-pages/sensor-replay-item.cpp
      android/skin/qt/extended-pages/sensor-replay-page.cpp
      android/skin/qt/extended-pages/sensor-replay-page.ui
  DEPS android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::common
       ui::gl-qt-bridge
       ui::widgets)

if(QTWEBENGINE)
  message(STATUS "Building fancy location ui")
  android_qt_lib(
    TARGET ext-page-location-web
    ALIAS ext-page-location-ui
    # Remove this once you are ready for shared libs. DLLEXPORT LOCATION_EXPORTS
    # DLLENABLE AEMU_LOCATION_SHARED SHARED
    SRC android/skin/qt/extended-pages/location-page-point.cpp
        android/skin/qt/extended-pages/location-page-route-playback.cpp
        android/skin/qt/extended-pages/location-page-route.cpp
        android/skin/qt/extended-pages/location-page.cpp
        android/skin/qt/extended-pages/location-page.ui
        android/skin/qt/extended-pages/location-route-playback-item.h
        android/skin/qt/extended-pages/network-connectivity-manager.cpp
        android/skin/qt/websockets/websocketclientwrapper.cpp
        android/skin/qt/websockets/websockettransport.cpp
    # We should move the resourcess here if this becomes a shared lib.
    DEPS android-emu
         android-emu-curl
         android-emu-location
         protobuf::libprotobuf
         qemu-host-common-headers
         Qt${QT_VERSION_MAJOR}::Network
         Qt${QT_VERSION_MAJOR}::WebChannel
         Qt${QT_VERSION_MAJOR}::WebEngineWidgets
         Qt${QT_VERSION_MAJOR}::WebSockets
         Qt${QT_VERSION_MAJOR}::Widgets
         ui::common
         ui::gl-qt-bridge
         ui::widgets)

  if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(ext-page-location-web
                          PRIVATE Qt${QT_VERSION_MAJOR}::WebEngineCore)
  endif()
  target_compile_definitions(ext-page-location-web PUBLIC USE_WEBENGINE)
else()
  message(STATUS "Building simple location ui")
  android_qt_lib(
    TARGET ext-page-location-no-web
    ALIAS ext-page-location-ui
    # Remove this once you are ready for shared libs. DLLEXPORT LOCATION_EXPORTS
    # DLLENABLE AEMU_LOCATION_SHARED SHARED
    SRC android/skin/qt/extended-pages/location-page-point.cpp
        android/skin/qt/extended-pages/location-page-route-playback.cpp
        android/skin/qt/extended-pages/location-page-route.cpp
        android/skin/qt/extended-pages/location-page.cpp
        android/skin/qt/extended-pages/location-page_noMaps.ui
    DEPS android-emu
         android-emu-curl
         android-emu-location
         protobuf::libprotobuf
         qemu-host-common-headers
         Qt${QT_VERSION_MAJOR}::Core
         Qt${QT_VERSION_MAJOR}::Widgets
         ui::common
         ui::gl-qt-bridge
         ui::widgets)
endif()

android_qt_lib(
  TARGET aemu-ui-qt
  ALIAS emulator-libui LICENSE Apache-2.0
  SRC android/skin/EmulatorSkin.cpp
      android/skin/LibuiAgent.cpp
      android/skin/qt/emulator-container.cpp
      android/skin/qt/emulator-overlay.cpp
      android/skin/qt/emulator-qt-window.cpp
      android/skin/qt/event-qt.cpp
      android/skin/qt/extended-window.cpp
      android/skin/qt/extended.ui
      android/skin/qt/FramelessDetector.cpp
      android/skin/qt/init-qt.cpp
      android/skin/qt/ModalOverlay.cpp
      android/skin/qt/mouse-event-handler.cpp
      android/skin/qt/multi-display-widget.cpp
      android/skin/qt/native-event-filter-factory.cpp
      android/skin/qt/native-keyboard-event-handler.cpp
      android/skin/qt/OverlayMessageCenter.cpp
      android/skin/qt/posture-selection-dialog.cpp
      android/skin/qt/postureselectiondialog.ui
      android/skin/qt/QtLogger.cpp
      android/skin/qt/QtLooper.cpp
      android/skin/qt/QtLooperImpl.h
      android/skin/qt/resizable-dialog.cpp
      android/skin/qt/resizable-dialog.ui
      android/skin/qt/resources.qrc
      android/skin/qt/screen-mask.cpp
      android/skin/qt/static_resources.qrc
      android/skin/qt/stylesheet.cpp
      android/skin/qt/surface-qt.cpp
      android/skin/qt/tool-window.cpp
      android/skin/qt/tools.ui
      android/skin/qt/virtualscene-control-window.cpp
      android/skin/qt/virtualscene-controls.ui
      android/skin/qt/virtualscene-info-dialog.ui
      android/skin/qt/VirtualSceneInfoDialog.cpp
      android/skin/qt/winsys-qt.cpp
      android/window-agent-impl.cpp
  DEPS aemu::recording
       android-emu
       android-emu-base
       android-emu-curl
       android-emu-location
       android-emu-protobuf
       android-hw-config
       ext-page-battery
       ext-page-bugreport
       ext-page-camera
       ext-page-car
       ext-page-cellular
       ext-page-display
       ext-page-dpad
       ext-page-finger
       ext-page-google-play
       ext-page-help
       ext-page-location-ui
       ext-page-microphone
       ext-page-recording
       ext-page-rotary
       ext-page-sensor-replay
       ext-page-settings
       ext-page-snapshot
       ext-page-telephony
       ext-page-tv-remote
       protobuf::libprotobuf
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Gui
       Qt${QT_VERSION_MAJOR}::Svg
       ui::common
       ui::gl-qt-bridge
       ui::widgets
       webrtc-yuv
       zlib)

if(DARWIN_X86_64 OR DARWIN_AARCH64)
  target_sources(aemu-ui-qt
                 PRIVATE src/android/skin/qt/mac-native-event-filter.mm)

  target_link_libraries(aemu-ui-qt PRIVATE "-framework Carbon")
endif()

if(WINDOWS_MSVC_X86_64)
  target_sources(
    aemu-ui-qt PRIVATE src/android/skin/qt/windows-native-window.cpp
                       src/android/skin/qt/windows-native-window.h)
  target_link_libraries(aemu-ui-qt PUBLIC dirent-win32)

  # Target specific compiler flags for windows, since we include FFMPEG C
  # sources from C++ we need to make sure this flag is set for c++ sources.
  target_compile_options(
    aemu-ui-qt PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-Wno-literal-suffix>")
endif()
android_target_link_libraries(aemu-ui-qt linux PRIVATE -lX11)
android_target_link_libraries(aemu-ui-qt linux-aarch64 PRIVATE -lX11 -lXau ${QT5_LINK_PATH})


if(NOT BUILDING_FOR_AARCH64)
  target_compile_definitions(aemu-ui-qt PRIVATE USE_MMX=1)
  target_compile_options(aemu-ui-qt PRIVATE "-mmmx")
endif()

if(QT_VERSION_MAJOR EQUAL 6)
  target_link_libraries(aemu-ui-qt PRIVATE Qt${QT_VERSION_MAJOR}::SvgWidgets)
endif()

# Linux compiler settings
android_target_compile_options(
  aemu-ui-qt linux-x86_64
  PRIVATE "-Wno-reserved-user-defined-literal" "-Wno-pointer-bool-conversion"
          "-Wno-deprecated-declarations" "-Wno-inconsistent-missing-override"
          "-Wno-return-type-c-linkage" "-Wno-invalid-constexpr")
