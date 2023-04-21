# The list of dependencies.
prebuilt(QT5)
prebuilt(FFMPEG)
message(STATUS "Building UI using Qt ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}")

# ~~~
# ! android_qt_lib: Create a target with the given sources and dependencies
#   it will automatically export the _autogen include directory for all the files
#   for which uic and moc are run.
#
# ``TARGET``    The name of the library
# ``DLLEXPORT``  Define used to mark export/import
# ``SHARED``     Present if this should be a shared library
# ``ALIAS``      Alias under which this lib is known
# ``SRC``        The set of sources
# ``DEPS``       List of private dependencies needed to build this library.
# ~~
function(android_qt_lib)
  # Parse arguments
  set(options SHARED)
  set(oneValueArgs TARGET DLLEXPORT DLLENABLE ALIAS)
  set(multiValueArgs SRC DEPS)
  cmake_parse_arguments(ext "${options}" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})

  if(${ext_SHARED})
    android_add_library(TARGET ${ext_TARGET} SHARED LICENSE Apache-2.0
                        SRC ${ext_SRC})
    if(WINDOWS_MSVC_X86_64)
      target_compile_definitions(${ext_TARGET} PUBLIC ${ext_DLLENABLE}
                                 PRIVATE ${ext_DLLEXPORT})
    endif()
  else()
    android_add_library(TARGET ${ext_TARGET} LICENSE Apache-2.0 SRC ${ext_SRC})
  endif()
  if(NOT "${ext_ALIAS}" STREQUAL "")
    set_property(GLOBAL APPEND PROPERTY ALIAS_LST
                                        "${ext_ALIAS}|${ext_TARGET}\n")
    add_library(${ext_ALIAS} ALIAS ${ext_TARGET})
  endif()
  target_link_libraries(${ext_TARGET} PRIVATE ${ext_DEPS})
  target_include_directories(
    ${ext_TARGET}
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/${ext_TARGET}_autogen/include PRIVATE .)

  if(NOT BUILDING_FOR_AARCH64)
    target_compile_definitions(${ext_TARGET} PRIVATE USE_MMX=1)
    target_compile_options(${ext_TARGET} PRIVATE "-mmmx")
  endif()

  if(OPTION_GFXSTREAM_BACKEND)
    target_compile_definitions(${ext_TARGET} PUBLIC AEMU_GFXSTREAM_BACKEND=1)
  endif()

endfunction()

android_qt_lib(
  TARGET emulator-ui-gl-bridge
  SRC android/gpu_frame.cpp android/skin/qt/gl-canvas.cpp
      android/skin/qt/gl-common.cpp android/skin/qt/gl-texture-draw.cpp
      android/skin/qt/gl-widget.cpp android/skin/qt/gl-widget.h
  DEPS android-emu
       android-emu-base
       android-emu-base-headers
       android-emu-utils
       emugl_common
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets)
target_include_directories(emulator-ui-gl-bridge PRIVATE .)
target_compile_options(emulator-ui-gl-bridge PRIVATE -Wno-return-type-c-linkage)
android_target_link_libraries(emulator-ui-gl-bridge linux-x86_64 PRIVATE -lX11)
android_target_link_libraries(emulator-ui-gl-bridge linux-aarch64
                              PRIVATE -lpulse -lX11 -lxcb -lXau)

# This libary contains a set of functions that cross the qemu boundary.
android_qt_lib(
  TARGET emulator-ui-base
  SRC android/skin/generic-event-buffer.cpp
      android/skin/generic-event.cpp
      android/skin/keycode-buffer.c
      android/skin/keycode.c
      android/skin/qt/utils/common.cpp
      # android/skin/rect.c
      android/skin/resource.c
  DEPS android-emu Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

android_add_library(
  TARGET emulator-recording
  LICENSE Apache-2.0
  SRC android/emulation/control/ScreenCapturer.cpp
      android/mp4/MP4Dataset.cpp
      android/mp4/MP4Demuxer.cpp
      android/mp4/SensorLocationEventProvider.cpp
      android/mp4/VideoMetadataProvider.cpp
      android/recording/audio/AudioProducer.cpp
      android/recording/codecs/audio/VorbisCodec.cpp
      android/recording/codecs/video/VP9Codec.cpp
      android/recording/FfmpegRecorder.cpp
      android/recording/Frame.cpp
      android/recording/GifConverter.cpp
      android/recording/video/GuestReadbackWorker.cpp
      android/recording/video/player/Clock.cpp
      android/recording/video/player/FrameQueue.cpp
      android/recording/video/player/PacketQueue.cpp
      android/recording/video/player/VideoPlayer.cpp
      android/recording/video/player/VideoPlayerNotifier.cpp
      android/recording/video/VideoFrameSharer.cpp
      android/recording/video/VideoProducer.cpp)
target_link_libraries(
  emulator-recording PRIVATE android-emu webrtc-yuv qemu-host-common-headers
                             android-emu-base-headers FFMPEG::FFMPEG)

android_qt_lib(
  TARGET emulator-ui-widgets
  SRC android/resource.c
      android/skin/EmulatorSkin.cpp
      android/skin/qt/angle-input-widget.cpp
      android/skin/qt/angle-input-widget.h
      android/skin/qt/common-controls/cc-list-item.cpp
      android/skin/qt/common-controls/cc-list-item.h
      android/skin/qt/common-controls/cc-list-item.ui
      android/skin/qt/editable-slider-widget.cpp
      android/skin/qt/editable-slider-widget.h
      android/skin/qt/error-dialog.cpp
      android/skin/qt/event-capturer.cpp
      android/skin/qt/event-capturer.h
      android/skin/qt/event-serializer.cpp
      android/skin/qt/event-subscriber.cpp
      android/skin/qt/event-subscriber.h
      android/skin/qt/extended-pages/common.cpp
      android/skin/qt/FramelessDetector.cpp
      android/skin/qt/poster-image-well.cpp
      android/skin/qt/poster-image-well.h
      android/skin/qt/poster-image-well.ui
      android/skin/qt/posture-selection-dialog.cpp
      android/skin/qt/posture-selection-dialog.h
      android/skin/qt/postureselectiondialog.ui
      android/skin/qt/qt-ui-commands.cpp
      android/skin/qt/QtThreading.cpp
      android/skin/qt/raised-material-button.h
      android/skin/qt/resizable-dialog.cpp
      android/skin/qt/resizable-dialog.h
      android/skin/qt/resizable-dialog.ui
      android/skin/qt/size-tweaker.cpp
      android/skin/qt/size-tweaker.h
      android/skin/qt/stylesheet.cpp
      android/skin/qt/tools.ui
      android/skin/qt/ui-event-recorder.cpp
      android/skin/qt/user-actions-counter.cpp
      android/skin/qt/user-actions-counter.h
      android/skin/qt/wavefront-obj-parser.cpp
  DEPS android-emu android-emu-base android-emu-base-headers
       Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

android_qt_lib(
  TARGET ext-page-camera
  SRC android/skin/qt/device-3d-widget.cpp
      android/skin/qt/device-3d-widget.h
      android/skin/qt/extended-pages/camera-page.cpp
      android/skin/qt/extended-pages/camera-page.h
      android/skin/qt/extended-pages/camera-page.ui
      android/skin/qt/extended-pages/camera-virtualscene-subpage.cpp
      android/skin/qt/extended-pages/camera-virtualscene-subpage.h
      android/skin/qt/extended-pages/camera-virtualscene-subpage.ui
      android/skin/qt/extended-pages/virtual-sensors-page.cpp
      android/skin/qt/extended-pages/virtual-sensors-page.h
      android/skin/qt/extended-pages/virtual-sensors-page.ui
  DEPS android-emu emulator-ui-gl-bridge emulator-ui-widgets
       qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Svg)

android_qt_lib(
  TARGET ext-page-car
  SRC android/skin/qt/car-cluster-window.cpp
      android/skin/qt/extended-pages/car-cluster-connector/car-cluster-connector.cpp
      android/skin/qt/extended-pages/car-data-emulation/car-property-utils.cpp
      android/skin/qt/extended-pages/car-data-emulation/car-property-utils.h
      android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.cpp
      android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.h
      android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.ui
      android/skin/qt/extended-pages/car-data-emulation/checkbox-dialog.cpp
      android/skin/qt/extended-pages/car-data-emulation/vhal-item.cpp
      android/skin/qt/extended-pages/car-data-emulation/vhal-table.cpp
      android/skin/qt/extended-pages/car-data-page.cpp
      android/skin/qt/extended-pages/car-data-page.h
      android/skin/qt/extended-pages/car-data-page.ui
      android/skin/qt/extended-pages/car-rotary-page.cpp
      android/skin/qt/extended-pages/car-rotary-page.h
      android/skin/qt/extended-pages/car-rotary-page.ui
      android/skin/qt/extended-pages/instr-cluster-render/car-cluster-widget.cpp
  DEPS android-emu
       emulator-ui-base
       emulator-ui-base
       emulator-ui-widgets
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets)

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
      android/skin/qt/extended-pages/record-macro-saved-item.h
      android/skin/qt/extended-pages/record-screen-page-tasks.h
      android/skin/qt/extended-pages/record-screen-page.cpp
      android/skin/qt/extended-pages/record-screen-page.h
      android/skin/qt/extended-pages/record-screen-page.ui
      android/skin/qt/extended-pages/record-settings-page.cpp
      android/skin/qt/extended-pages/record-settings-page.h
      android/skin/qt/extended-pages/record-settings-page.ui
      android/skin/qt/video-player/QtVideoPlayerNotifier.cpp
      android/skin/qt/video-player/QtVideoPlayerNotifier.h
      android/skin/qt/video-player/VideoInfo.cpp
      android/skin/qt/video-player/VideoInfo.h
      android/skin/qt/video-player/VideoPlayerWidget.cpp
      android/skin/qt/video-player/VideoPlayerWidget.h
  DEPS android-emu emulator-recording emulator-ui-widgets
       Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

android_qt_lib(
  TARGET ext-page-bugreport
  SRC android/skin/qt/extended-pages/bug-report-page.cpp
      android/skin/qt/extended-pages/bug-report-page.h
      android/skin/qt/extended-pages/bug-report-page.ui
  DEPS android-emu emulator-ui-base emulator-ui-widgets
       qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets)

android_qt_lib(
  TARGET ext-page-battery
  SRC android/skin/qt/extended-pages/battery-page.cpp
      android/skin/qt/extended-pages/battery-page.h
      android/skin/qt/extended-pages/battery-page.ui
  DEPS android-emu emulator-ui-widgets Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets)

android_qt_lib(
  TARGET ext-page-cellular
  SRC android/skin/qt/extended-pages/cellular-page.cpp
      android/skin/qt/extended-pages/cellular-page.h
      android/skin/qt/extended-pages/cellular-page.ui
  DEPS android-emu emulator-ui-widgets qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

android_qt_lib(
  TARGET ext-page-dpad
  SRC android/skin/qt/extended-pages/dpad-page.cpp
      android/skin/qt/extended-pages/dpad-page.h
      android/skin/qt/extended-pages/dpad-page.ui
  DEPS android-emu emulator-ui-widgets qemu-host-common-headers
       qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets)

android_qt_lib(
  TARGET ext-page-finger
  SRC android/skin/qt/extended-pages/finger-page.cpp
      android/skin/qt/extended-pages/finger-page.h
      android/skin/qt/extended-pages/finger-page.ui
  DEPS android-emu emulator-ui-widgets qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

android_qt_lib(
  TARGET ext-page-google-play
  SRC android/skin/qt/extended-pages/google-play-page.cpp
      android/skin/qt/extended-pages/google-play-page.h
      android/skin/qt/extended-pages/google-play-page.ui
  DEPS android-emu emulator-ui-base emulator-ui-widgets
       qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets)

android_qt_lib(
  TARGET ext-page-help
  SRC android/skin/qt/extended-pages/help-page.cpp
      android/skin/qt/extended-pages/help-page.h
      android/skin/qt/extended-pages/help-page.ui
  DEPS android-emu emulator-ui-widgets qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

android_qt_lib(
  TARGET ext-page-microphone
  SRC android/skin/qt/extended-pages/microphone-page.cpp
      android/skin/qt/extended-pages/microphone-page.h
      android/skin/qt/extended-pages/microphone-page.ui
  DEPS android-emu emulator-ui-widgets qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

android_qt_lib(
  TARGET ext-page-display
  SRC android/skin/qt/extended-pages/multi-display-arrangement.cpp
      android/skin/qt/extended-pages/multi-display-arrangement.h
      android/skin/qt/extended-pages/multi-display-item.cpp
      android/skin/qt/extended-pages/multi-display-page.cpp
      android/skin/qt/extended-pages/multi-display-page.h
      android/skin/qt/extended-pages/multi-display-page.ui
  DEPS android-emu emulator-ui-widgets qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

android_qt_lib(
  TARGET ext-page-rotary
  SRC android/skin/qt/extended-pages/rotary-input-dial.cpp
      android/skin/qt/extended-pages/rotary-input-dial.h
      android/skin/qt/extended-pages/rotary-input-page.cpp
      android/skin/qt/extended-pages/rotary-input-page.h
      android/skin/qt/extended-pages/rotary-input-page.ui
  DEPS android-emu emulator-ui-widgets qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Svg)

android_qt_lib(
  TARGET ext-page-settings
  SRC android/skin/qt/extended-pages/perfstats-page.cpp
      android/skin/qt/extended-pages/perfstats-page.h
      android/skin/qt/extended-pages/perfstats-page.ui
      android/skin/qt/extended-pages/settings-page-proxy.cpp
      android/skin/qt/extended-pages/settings-page.cpp
      android/skin/qt/extended-pages/settings-page.h
      android/skin/qt/extended-pages/settings-page.ui
      android/skin/qt/perf-stats-3d-widget.cpp
  DEPS android-emu emulator-ui-gl-bridge emulator-ui-widgets
       qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets)

android_qt_lib(
  TARGET ext-page-snapshot
  SRC android/skin/qt/extended-pages/snapshot-page.cpp
      android/skin/qt/extended-pages/snapshot-page.h
      android/skin/qt/extended-pages/snapshot-page.ui
  DEPS android-emu emulator-ui-widgets qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

android_qt_lib(
  TARGET ext-page-telephony
  SRC android/skin/qt/extended-pages/telephony-page.cpp
      android/skin/qt/extended-pages/telephony-page.h
      android/skin/qt/extended-pages/telephony-page.ui
  DEPS android-emu emulator-ui-widgets qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

android_qt_lib(
  TARGET ext-page-tv-remote
  SRC android/skin/qt/extended-pages/tv-remote-page.cpp
      android/skin/qt/extended-pages/tv-remote-page.h
      android/skin/qt/extended-pages/tv-remote-page.ui
  DEPS android-emu emulator-ui-widgets qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

android_qt_lib(
  TARGET ext-page-sensor-replay
  SRC android/skin/qt/extended-pages/car-data-emulation/car-property-utils.cpp
      android/skin/qt/extended-pages/car-data-emulation/car-property-utils.h
      android/skin/qt/extended-pages/sensor-replay-item.cpp
      android/skin/qt/extended-pages/sensor-replay-item.h
      android/skin/qt/extended-pages/sensor-replay-page.cpp
      android/skin/qt/extended-pages/sensor-replay-page.h
      android/skin/qt/extended-pages/sensor-replay-page.ui
  DEPS android-emu
       android-emu-agents
       emulator-ui-base
       emulator-ui-widgets
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets)

if(QTWEBENGINE)
  message(STATUS "Building fancy location ui")
  android_qt_lib(
    TARGET ext-page-location-web ALIAS ext-page-location-ui
    # Remove this once you are ready for shared libs. DLLEXPORT LOCATION_EXPORTS
    # DLLENABLE AEMU_LOCATION_SHARED SHARED
    SRC android/skin/qt/extended-pages/location-page-point.cpp
        android/skin/qt/extended-pages/location-page-route-playback.cpp
        android/skin/qt/extended-pages/location-page-route.cpp
        android/skin/qt/extended-pages/location-page.cpp
        android/skin/qt/extended-pages/location-page.h
        android/skin/qt/extended-pages/location-page.ui
        android/skin/qt/extended-pages/location-route-playback-item.h
        android/skin/qt/extended-pages/network-connectivity-manager.cpp
        android/skin/qt/websockets/websocketclientwrapper.cpp
        android/skin/qt/websockets/websockettransport.cpp
    DEPS android-emu
         android-emu-base
         android-emu-curl
         android-emu-location
         android-emu-protobuf
         emulator-ui-base
         emulator-ui-widgets
         protobuf::libprotobuf
         Qt${QT_VERSION_MAJOR}::Network
         Qt${QT_VERSION_MAJOR}::WebChannel
         Qt${QT_VERSION_MAJOR}::WebEngineWidgets
         Qt${QT_VERSION_MAJOR}::WebSockets
         Qt${QT_VERSION_MAJOR}::Widgets)

  if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(ext-page-location-web
                          PRIVATE Qt${QT_VERSION_MAJOR}::WebEngineCore)
  endif()
  target_compile_definitions(ext-page-location-web PUBLIC USE_WEBENGINE)
else()
  message(STATUS "Building simple location ui")
  android_qt_lib(
    TARGET ext-page-location-no-web ALIAS ext-page-location-ui
    # Remove this once you are ready for shared libs. DLLEXPORT LOCATION_EXPORTS
    # DLLENABLE AEMU_LOCATION_SHARED SHARED
    SRC android/skin/qt/extended-pages/location-page-point.cpp
        android/skin/qt/extended-pages/location-page-route-playback.cpp
        android/skin/qt/extended-pages/location-page-route.cpp
        android/skin/qt/extended-pages/location-page.cpp
        android/skin/qt/extended-pages/location-page.h
        android/skin/qt/extended-pages/location-page_noMaps.ui
        android/skin/qt/extended-pages/location-route-playback-item.h
    DEPS android-emu
         android-emu-base
         android-emu-curl
         android-emu-location
         android-emu-protobuf
         emulator-ui-base
         emulator-ui-widgets
         protobuf::libprotobuf
         Qt${QT_VERSION_MAJOR}::Core
         Qt${QT_VERSION_MAJOR}::Widgets)
endif()

android_qt_lib(
  TARGET emulator-libui
         # TODO(jansene): Uncomment these if you wish to make these fully
         # shared. This will require some additional refactoring on windows.
         # DLLEXPORT UI_EXPORTS DLLENABLE AEMU_UI_SHARED SHARED
         LICENSE
         Apache-2.0
  SRC android/emulator-window.c
      android/framebuffer.c
      android/main-common-ui.c
      android/recording/screen-recorder.cpp
      android/skin/charmap.c
      android/skin/file.c
      android/skin/image.c
      android/skin/keyboard.c
      android/skin/LibuiAgent.cpp
      android/skin/qt/emulator-container.cpp
      android/skin/qt/emulator-container.h
      android/skin/qt/emulator-no-qt-no-window.cpp
      android/skin/qt/emulator-no-qt-no-window.h
      android/skin/qt/emulator-overlay.cpp
      android/skin/qt/emulator-overlay.h
      android/skin/qt/emulator-qt-window.cpp
      android/skin/qt/emulator-qt-window.h
      android/skin/qt/event-qt.cpp
      android/skin/qt/extended-window.cpp
      android/skin/qt/extended-window.h
      android/skin/qt/extended.ui
      android/skin/qt/init-qt.cpp
      android/skin/qt/ModalOverlay.cpp
      android/skin/qt/ModalOverlay.h
      android/skin/qt/mouse-event-handler.cpp
      android/skin/qt/multi-display-widget.cpp
      android/skin/qt/multi-display-widget.h
      android/skin/qt/native-event-filter-factory.cpp
      android/skin/qt/native-keyboard-event-handler.cpp
      android/skin/qt/OverlayMessageCenter.cpp
      android/skin/qt/OverlayMessageCenter.h
      android/skin/qt/QtLogger.cpp
      android/skin/qt/QtLooper.cpp
      android/skin/qt/QtLooperImpl.h
      android/skin/qt/resources.qrc
      android/skin/qt/screen-mask.cpp
      android/skin/qt/screen-mask.h
      android/skin/qt/static_resources.qrc
      android/skin/qt/surface-qt.cpp
      android/skin/qt/tool-window.cpp
      android/skin/qt/tool-window.h
      android/skin/qt/virtualscene-control-window.cpp
      android/skin/qt/virtualscene-control-window.h
      android/skin/qt/virtualscene-controls.ui
      android/skin/qt/virtualscene-info-dialog.ui
      android/skin/qt/VirtualSceneInfoDialog.cpp
      android/skin/qt/VirtualSceneInfoDialog.h
      android/skin/qt/winsys-qt.cpp
      android/skin/trackball.c
      android/skin/ui.c
      android/skin/user-config.c
      android/skin/window.c
      android/window-agent-impl.cpp
  DEPS android-emu
       android-emu-agents
       android-emu-avd
       android-emu-base
       android-emu-base-headers
       android-emu-curl
       android-emu-feature
       android-emu-location
       android-emu-protobuf
       android-hw-config
       emulator-recording
       emulator-ui-base
       emulator-ui-gl-bridge
       emulator-ui-widgets
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
       webrtc-yuv
       zlib)

if(DARWIN_AARCH64 OR DARWIN_X86_64)
  target_sources(
    emulator-libui PRIVATE android/skin/qt/mac-native-event-filter.mm
                           android/skin/qt/mac-native-window.mm)
endif()
if(WINDOWS_MSVC_X86_64)
  target_sources(emulator-libui
                 PRIVATE android/skin/qt/windows-native-window.cpp)
  # Windows-msvc specific dependencies. Need these for posix support.
  target_link_libraries(emulator-libui PUBLIC dirent-win32)

  # Target specific compiler flags for windows, since we include FFMPEG C
  # sources from C++ we need to make sure this flag is set for c++ sources.
  target_compile_options(
    emulator-libui PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-Wno-literal-suffix>")
endif()

# android_install_shared(emulator-libui)
android_target_link_libraries(emulator-libui darwin PRIVATE "-framework Carbon")
android_target_link_libraries(emulator-libui linux PRIVATE "-lX11")

if(NOT BUILDING_FOR_AARCH64)
  target_compile_definitions(emulator-libui PRIVATE USE_MMX=1)
  target_compile_options(emulator-libui PRIVATE "-mmmx")
endif()

if(QT_VERSION_MAJOR EQUAL 6)
  target_link_libraries(emulator-libui
                        PRIVATE Qt${QT_VERSION_MAJOR}::SvgWidgets)
endif()

# Linux compiler settings
android_target_compile_options(
  emulator-libui linux-x86_64
  PRIVATE "-Wno-reserved-user-defined-literal" "-Wno-pointer-bool-conversion"
          "-Wno-deprecated-declarations" "-Wno-inconsistent-missing-override"
          "-Wno-return-type-c-linkage" "-Wno-invalid-constexpr")

# Mac Os compiler settings
android_target_compile_options(
  emulator-libui darwin-x86_64
  PRIVATE "-Wno-reserved-user-defined-literal" "-Wno-pointer-bool-conversion"
          "-Wno-deprecated-declarations" "-Wno-inconsistent-missing-override"
          "-Wno-return-type-c-linkage" "-Wno-invalid-constexpr")

list(APPEND android-libui-testdata testdata/mp4/video.mp4)

if(NOT LINUX_AARCH64)
  android_add_test(
    TARGET emulator-libui_unittests
    SRC android/mp4/MP4Dataset_test.cpp
        android/mp4/MP4Demuxer_test.cpp
        android/mp4/SensorLocationEventProvider_test.cpp
        android/mp4/VideoMetadataProvider_test.cpp
        android/recording/FfmpegRecorder.cpp
        android/recording/test/DummyAudioProducer.cpp
        android/recording/test/DummyVideoProducer.cpp
        android/recording/test/FfmpegRecorder_unittest.cpp
        android/skin/keycode-buffer_unittest.cpp
        android/skin/keycode_unittest.cpp
        android/skin/qt/native-keyboard-event-handler_unittest.cpp
        android/skin/qt/qtmain_dummy_test.cpp
        # android/skin/rect_unittest.cpp
  )

  android_copy_test_files(emulator-libui_unittests "${android-libui-testdata}"
                          testdata)

  target_compile_options(emulator-libui_unittests
                         PRIVATE -O0 -UNDEBUG -Wno-deprecated-declarations)

  # Target specific compiler flags for windows
  android_target_compile_options(
    emulator-libui_unittests windows
    PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-Wno-literal-suffix>")

  android_target_dependency(emulator-libui_unittests all
                            QT5_SHARED_DEPENDENCIES)
  android_target_properties(emulator-libui_unittests all
                            "${QT5_SHARED_PROPERTIES}")

  target_link_libraries(
    emulator-libui_unittests
    PRIVATE emulator-libui android-emu emulator-ui-base emulator-recording
            gmock_main)

  android_target_link_libraries(emulator-libui_unittests windows_msvc-x86_64
                                PUBLIC dirent-win32)

endif()

# Version of libui without Qt
android_add_library(
  TARGET emulator-libui-headless
  LICENSE Apache-2.0
  SRC android/emulation/control/ScreenCapturer.cpp
      android/emulator-window.c
      android/framebuffer.c
      android/gpu_frame.cpp
      android/main-common-ui.c
      android/recording/screen-recorder.cpp
      android/resource.c
      android/skin/charmap.c
      android/skin/event-headless.cpp
      android/skin/file.c
      android/skin/generic-event-buffer.cpp
      android/skin/generic-event.cpp
      android/skin/image.c
      android/skin/keyboard.c
      android/skin/keycode-buffer.c
      android/skin/keycode.c
      android/skin/LibuiAgent.cpp
      android/skin/qt/emulator-no-qt-no-window.cpp
      # android/skin/rect.c
      android/skin/resource.c
      android/skin/surface-headless.cpp
      android/skin/trackball.c
      android/skin/ui.c
      android/skin/user-config.c
      android/skin/window.c
      android/skin/winsys-headless.cpp
      android/window-agent-headless-impl.cpp
  DEPS android-emu
       android-emu-avd
       android-emu-feature
       android-hw-config
       emulator-recording
       qemu-host-common-headers
       webrtc-yuv
       zlib)

target_compile_definitions(emulator-libui-headless PRIVATE -DCONFIG_HEADLESS)
if(NOT LINUX_AARCH64)
  target_compile_options(emulator-libui-headless PRIVATE "-DUSE_MMX=1" "-mmmx")
endif()

if(WINDOWS_MSVC_X86_64)
  # Qt in windows will call main from win-main v.s. calling qt_main. we have to
  # make a separate launch library to make sure that we do not end up with
  # duplicate main symbols when linking emulator-libui (it used to work do to a
  # cmake linker quirk).
  android_add_library(TARGET emulator-winqt-launcher LICENSE Apache-2.0
                      SRC android/skin/qt/windows-qt-launcher.cpp)
  target_link_libraries(emulator-winqt-launcher PRIVATE android-emu-base)
endif()
