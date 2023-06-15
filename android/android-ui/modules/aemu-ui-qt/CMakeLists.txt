android_qt_lib(
  TARGET aemu-ui-qt-base
  ALIAS emulator-libui LICENSE Apache-2.0
  INCLUDES
  src
  SRC android/skin/EmulatorSkin.cpp android/skin/qt/QtLogger.cpp
      android/skin/qt/QtLooper.cpp android/skin/qt/QtLooperImpl.h
  DEPS android-emu-agents
       android-emu-avd
       android-emu-base
       android-emu-base-headers
       android-emu-crashreport
       android-emu-files
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Gui
       Qt${QT_VERSION_MAJOR}::Svg
       ui::widgets)

android_qt_lib(
  TARGET aemu-ui-qt
  ALIAS emulator-libui LICENSE Apache-2.0
  SRC android/skin/LibuiAgent.cpp
      android/skin/qt/emulator-container.cpp
      android/skin/qt/emulator-overlay.cpp
      android/skin/qt/emulator-qt-window.cpp
      android/skin/qt/event-qt.cpp
      android/skin/qt/extended-window-grpc.cpp
      android/skin/qt/extended-window-grpc.ui
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
  DEPS aemu-ui-car-property-utils
       aemu-ui-qt-base
       aemu::recording
       android-emu
       android-emu-base
       android-emu-curl
       android-emu-location
       android-emu-protobuf
       android-hw-config
       ext-page-battery
       ext-page-battery-grpc
       ext-page-bugreport
       ext-page-bugreport-grpc
       ext-page-camera
       ext-page-camera-grpc
       ext-page-car
       ext-page-cellular
       ext-page-cellular-grpc
       ext-page-display
       ext-page-dpad
       ext-page-finger
       ext-page-finger-grpc
       ext-page-google-play
       ext-page-help
       ext-page-location-ui
       ext-page-microphone
       ext-page-recording
       ext-page-rotary
       ext-page-sensor-replay
       ext-page-settings
       ext-page-snapshot
       ext-page-snapshot-grpc
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
android_target_link_libraries(aemu-ui-qt linux-aarch64 PRIVATE -lX11 -lXau
                                                               ${QT5_LINK_PATH})

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
