# The list of dependencies.
prebuilt(QT5)
prebuilt(FFMPEG)

set(ANDROID_LIBUI_HEADLESS_SRC_FILES
    # cmake-format: sortable
    android/emulation/control/ScreenCapturer.cpp
    android/emulator-window.c
    android/gpu_frame.cpp
    android/main-common-ui.c
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
    android/recording/screen-recorder.cpp
    android/recording/video/GuestReadbackWorker.cpp
    android/recording/video/player/Clock.cpp
    android/recording/video/player/FrameQueue.cpp
    android/recording/video/player/PacketQueue.cpp
    android/recording/video/player/VideoPlayer.cpp
    android/recording/video/player/VideoPlayerNotifier.cpp
    android/recording/video/VideoFrameSharer.cpp
    android/recording/video/VideoProducer.cpp
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
    android/skin/rect.c
    android/skin/resource.c
    android/skin/surface-headless.cpp
    android/skin/trackball.c
    android/skin/ui.c
    android/skin/window.c
    android/skin/winsys-headless.cpp
    android/window-agent-headless-impl.cpp)

set(ANDROID_LIBUI_SRC_FILES
    # cmake-format: sortable
    android/emulation/control/ScreenCapturer.cpp
    android/emulator-window.c
    android/gpu_frame.cpp
    android/main-common-ui.c
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
    android/recording/screen-recorder.cpp
    android/recording/video/GuestReadbackWorker.cpp
    android/recording/video/player/Clock.cpp
    android/recording/video/player/FrameQueue.cpp
    android/recording/video/player/PacketQueue.cpp
    android/recording/video/player/VideoPlayer.cpp
    android/recording/video/player/VideoPlayerNotifier.cpp
    android/recording/video/VideoFrameSharer.cpp
    android/recording/video/VideoProducer.cpp
    android/resource.c
    android/skin/charmap.c
    android/skin/file.c
    android/skin/generic-event-buffer.cpp
    android/skin/generic-event.cpp
    android/skin/image.c
    android/skin/keyboard.c
    android/skin/keycode-buffer.c
    android/skin/keycode.c
    android/skin/LibuiAgent.cpp
    android/skin/qt/angle-input-widget.cpp
    android/skin/qt/car-cluster-window.cpp
    android/skin/qt/common-controls/cc-list-item.cpp
    android/skin/qt/device-3d-widget.cpp
    android/skin/qt/editable-slider-widget.cpp
    android/skin/qt/emulator-container.cpp
    android/skin/qt/emulator-no-qt-no-window.cpp
    android/skin/qt/emulator-overlay.cpp
    android/skin/qt/emulator-qt-window.cpp
    android/skin/qt/error-dialog.cpp
    android/skin/qt/event-capturer.cpp
    android/skin/qt/event-qt.cpp
    android/skin/qt/event-serializer.cpp
    android/skin/qt/event-subscriber.cpp
    android/skin/qt/extended-pages/battery-page.cpp
    android/skin/qt/extended-pages/bug-report-page.cpp
    android/skin/qt/extended-pages/camera-page.cpp
    android/skin/qt/extended-pages/camera-virtualscene-subpage.cpp
    android/skin/qt/extended-pages/car-cluster-connector/car-cluster-connector.cpp
    android/skin/qt/extended-pages/car-data-emulation/car-property-utils.cpp
    android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.cpp
    android/skin/qt/extended-pages/car-data-emulation/checkbox-dialog.cpp
    android/skin/qt/extended-pages/car-data-emulation/vhal-item.cpp
    android/skin/qt/extended-pages/car-data-emulation/vhal-table.cpp
    android/skin/qt/extended-pages/car-data-page.cpp
    android/skin/qt/extended-pages/car-rotary-page.cpp
    android/skin/qt/extended-pages/cellular-page.cpp
    android/skin/qt/extended-pages/common.cpp
    android/skin/qt/extended-pages/dpad-page.cpp
    android/skin/qt/extended-pages/finger-page.cpp
    android/skin/qt/extended-pages/google-play-page.cpp
    android/skin/qt/extended-pages/help-page.cpp
    android/skin/qt/extended-pages/instr-cluster-render/car-cluster-widget.cpp
    android/skin/qt/extended-pages/location-page-point.cpp
    android/skin/qt/extended-pages/location-page-route-playback.cpp
    android/skin/qt/extended-pages/location-page-route.cpp
    android/skin/qt/extended-pages/location-page.cpp
    android/skin/qt/extended-pages/microphone-page.cpp
    android/skin/qt/extended-pages/multi-display-arrangement.cpp
    android/skin/qt/extended-pages/multi-display-item.cpp
    android/skin/qt/extended-pages/multi-display-page.cpp
    android/skin/qt/extended-pages/network-connectivity-manager.cpp
    android/skin/qt/extended-pages/perfstats-page.cpp
    android/skin/qt/extended-pages/record-and-playback-page.cpp
    android/skin/qt/extended-pages/record-macro-edit-dialog.cpp
    android/skin/qt/extended-pages/record-macro-page.cpp
    android/skin/qt/extended-pages/record-macro-saved-item.cpp
    android/skin/qt/extended-pages/record-screen-page.cpp
    android/skin/qt/extended-pages/record-settings-page.cpp
    android/skin/qt/extended-pages/rotary-input-dial.cpp
    android/skin/qt/extended-pages/rotary-input-page.cpp
    android/skin/qt/extended-pages/settings-page-proxy.cpp
    android/skin/qt/extended-pages/settings-page.cpp
    android/skin/qt/extended-pages/snapshot-page.cpp
    android/skin/qt/extended-pages/telephony-page.cpp
    android/skin/qt/extended-pages/tv-remote-page.cpp
    android/skin/qt/extended-pages/virtual-sensors-page.cpp
    android/skin/qt/extended-window.cpp
    android/skin/qt/FramelessDetector.cpp
    android/skin/qt/gl-canvas.cpp
    android/skin/qt/gl-common.cpp
    android/skin/qt/gl-texture-draw.cpp
    android/skin/qt/gl-widget.cpp
    android/skin/qt/init-qt.cpp
    android/skin/qt/ModalOverlay.cpp
    android/skin/qt/native-event-filter-factory.cpp
    android/skin/qt/native-keyboard-event-handler.cpp
    android/skin/qt/OverlayMessageCenter.cpp
    android/skin/qt/perf-stats-3d-widget.cpp
    android/skin/qt/poster-image-well.cpp
    android/skin/qt/posture-selection-dialog.cpp
    android/skin/qt/qt-ui-commands.cpp
    android/skin/qt/QtLooper.cpp
    android/skin/qt/QtThreading.cpp
    android/skin/qt/screen-mask.cpp
    android/skin/qt/size-tweaker.cpp
    android/skin/qt/stylesheet.cpp
    android/skin/qt/surface-qt.cpp
    android/skin/qt/tool-window.cpp
    android/skin/qt/ui-event-recorder.cpp
    android/skin/qt/user-actions-counter.cpp
    android/skin/qt/video-player/QtVideoPlayerNotifier.cpp
    android/skin/qt/video-player/VideoInfo.cpp
    android/skin/qt/video-player/VideoPlayerWidget.cpp
    android/skin/qt/virtualscene-control-window.cpp
    android/skin/qt/VirtualSceneInfoDialog.cpp
    android/skin/qt/wavefront-obj-parser.cpp
    android/skin/qt/winsys-qt.cpp
    android/skin/rect.c
    android/skin/resource.c
    android/skin/trackball.c
    android/skin/ui.c
    android/skin/window.c
    android/window-agent-impl.cpp)

# Note, these are separated for historical reasons only, the uic compiler will find this automatically
set(ANDROID_SKIN_QT_UI_SRC_FILES
    # cmake-format: sortable
    android/skin/qt/common-controls/cc-list-item.ui
    android/skin/qt/extended-pages/battery-page.ui
    android/skin/qt/extended-pages/bug-report-page.ui
    android/skin/qt/extended-pages/camera-page.ui
    android/skin/qt/extended-pages/camera-virtualscene-subpage.ui
    android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.ui
    android/skin/qt/extended-pages/car-data-page.ui
    android/skin/qt/extended-pages/car-rotary-page.ui
    android/skin/qt/extended-pages/cellular-page.ui
    android/skin/qt/extended-pages/dpad-page.ui
    android/skin/qt/extended-pages/finger-page.ui
    android/skin/qt/extended-pages/google-play-page.ui
    android/skin/qt/extended-pages/help-page.ui
    android/skin/qt/extended-pages/microphone-page.ui
    android/skin/qt/extended-pages/multi-display-page.ui
    android/skin/qt/extended-pages/record-and-playback-page.ui
    android/skin/qt/extended-pages/record-macro-edit-dialog.ui
    android/skin/qt/extended-pages/record-macro-page.ui
    android/skin/qt/extended-pages/record-macro-saved-item.ui
    android/skin/qt/extended-pages/record-screen-page.ui
    android/skin/qt/extended-pages/record-settings-page.ui
    android/skin/qt/extended-pages/rotary-input-page.ui
    android/skin/qt/extended-pages/settings-page.ui
    android/skin/qt/extended-pages/snapshot-page.ui
    android/skin/qt/extended-pages/telephony-page.ui
    android/skin/qt/extended-pages/tv-remote-page.ui
    android/skin/qt/extended-pages/virtual-sensors-page.ui
    android/skin/qt/extended.ui
    android/skin/qt/poster-image-well.ui
    android/skin/qt/postureselectiondialog.ui
    android/skin/qt/tools.ui
    android/skin/qt/virtualscene-controls.ui
    android/skin/qt/virtualscene-info-dialog.ui)

# Same here, the MOC compiler will find these files automatically.
set(ANDROID_SKIN_QT_MOC_SRC_FILES
    # cmake-format: sortable
    android/skin/qt/angle-input-widget.h
    android/skin/qt/common-controls/cc-list-item.h
    android/skin/qt/device-3d-widget.h
    android/skin/qt/editable-slider-widget.h
    android/skin/qt/emulator-container.h
    android/skin/qt/emulator-no-qt-no-window.h
    android/skin/qt/emulator-overlay.h
    android/skin/qt/emulator-qt-window.h
    android/skin/qt/event-capturer.h
    android/skin/qt/event-subscriber.h
    android/skin/qt/extended-pages/battery-page.h
    android/skin/qt/extended-pages/bug-report-page.h
    android/skin/qt/extended-pages/camera-page.h
    android/skin/qt/extended-pages/camera-virtualscene-subpage.h
    android/skin/qt/extended-pages/car-data-emulation/car-property-utils.h
    android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.h
    android/skin/qt/extended-pages/car-data-page.h
    android/skin/qt/extended-pages/car-rotary-page.h
    android/skin/qt/extended-pages/cellular-page.h
    android/skin/qt/extended-pages/dpad-page.h
    android/skin/qt/extended-pages/finger-page.h
    android/skin/qt/extended-pages/google-play-page.h
    android/skin/qt/extended-pages/help-page.h
    android/skin/qt/extended-pages/location-page.h
    android/skin/qt/extended-pages/location-route-playback-item.h
    android/skin/qt/extended-pages/microphone-page.h
    android/skin/qt/extended-pages/multi-display-arrangement.h
    android/skin/qt/extended-pages/multi-display-page.h
    android/skin/qt/extended-pages/record-and-playback-page.h
    android/skin/qt/extended-pages/record-macro-edit-dialog.h
    android/skin/qt/extended-pages/record-macro-page.h
    android/skin/qt/extended-pages/record-macro-saved-item.h
    android/skin/qt/extended-pages/record-screen-page-tasks.h
    android/skin/qt/extended-pages/record-screen-page.h
    android/skin/qt/extended-pages/record-settings-page.h
    android/skin/qt/extended-pages/rotary-input-dial.h
    android/skin/qt/extended-pages/rotary-input-page.h
    android/skin/qt/extended-pages/settings-page.h
    android/skin/qt/extended-pages/snapshot-page.h
    android/skin/qt/extended-pages/telephony-page.h
    android/skin/qt/extended-pages/tv-remote-page.h
    android/skin/qt/extended-pages/virtual-sensors-page.h
    android/skin/qt/extended-window.h
    android/skin/qt/gl-widget.h
    android/skin/qt/ModalOverlay.h
    android/skin/qt/OverlayMessageCenter.h
    android/skin/qt/poster-image-well.h
    android/skin/qt/posture-selection-dialog.h
    android/skin/qt/QtLooperImpl.h
    android/skin/qt/raised-material-button.h
    android/skin/qt/screen-mask.h
    android/skin/qt/size-tweaker.h
    android/skin/qt/tool-window.h
    android/skin/qt/user-actions-counter.h
    android/skin/qt/video-player/QtVideoPlayerNotifier.h
    android/skin/qt/video-player/VideoInfo.h
    android/skin/qt/video-player/VideoPlayerWidget.h
    android/skin/qt/virtualscene-control-window.h
    android/skin/qt/VirtualSceneInfoDialog.h)

# Resources will now always be compiled in.
set(ANDROID_SKIN_QT_DYNAMIC_RESOURCES android/skin/qt/resources.qrc)
set(ANDROID_SKIN_QT_UI_SRC_FILES android/skin/qt/static_resources.qrc)

set(ANDROID_SKIN_SOURCES
    # cmake-format: sortable
    android/skin/qt/event-qt.cpp android/skin/qt/init-qt.cpp android/skin/qt/QtLogger.cpp android/skin/qt/surface-qt.cpp
    android/skin/qt/winsys-qt.cpp)

# Set the target specific sources.
if(NOT QTWEBENGINE)
  message(STATUS "Webengine disabled.")
  set(emulator-libui_darwin-x86_64_src android/skin/qt/mac-native-window.mm android/skin/qt/mac-native-event-filter.mm
                                       android/skin/qt/extended-pages/location-page_noMaps.ui)
  set(emulator-libui_windows_msvc-x86_64_src android/skin/qt/windows-native-window.cpp
                                             android/skin/qt/extended-pages/location-page_noMaps.ui)
  set(emulator-libui_linux-x86_64_src android/skin/qt/extended-pages/location-page_noMaps.ui)
else()
  message(STATUS "Webengine enabled")
  set(emulator-libui_darwin-x86_64_src
      # cmake-format: sortable
      android/skin/qt/extended-pages/location-page.ui
      android/skin/qt/mac-native-event-filter.mm
      android/skin/qt/mac-native-window.mm
      android/skin/qt/websockets/websocketclientwrapper.cpp
      android/skin/qt/websockets/websockettransport.cpp)
  set(emulator-libui_windows_msvc-x86_64_src
      # cmake-format: sortable
      android/skin/qt/extended-pages/location-page.ui android/skin/qt/websockets/websocketclientwrapper.cpp
      android/skin/qt/websockets/websockettransport.cpp android/skin/qt/windows-native-window.cpp)
  set(emulator-libui_linux-x86_64_src
      # cmake-format: sortable
      android/skin/qt/extended-pages/location-page.ui android/skin/qt/websockets/websocketclientwrapper.cpp
      android/skin/qt/websockets/websockettransport.cpp)
endif()

# Set the sources
set(emulator-libui_src ${ANDROID_LIBUI_SRC_FILES} ${ANDROID_SKIN_QT_DYNAMIC_RESOURCES} ${ANDROID_SKIN_QT_MOC_SRC_FILES}
                       ${ANDROID_SKIN_QT_UI_SRC_FILES} ${ANDROID_SKIN_SOURCES})

android_add_library(
  TARGET emulator-libui LICENSE Apache-2.0 SRC # cmake-format: sortable
                                               ${emulator-libui_src} DARWIN ${emulator-libui_darwin-x86_64_src}
  LINUX ${emulator-libui_linux-x86_64_src} MSVC ${emulator-libui_windows_msvc-x86_64_src})

# TODO: Remove this and the "USE_WEBENGINE" defines once we have: --no-window mode has no dependency on Qt
if(QTWEBENGINE)
  target_compile_definitions(emulator-libui PRIVATE "-DUSE_WEBENGINE")
  target_link_libraries(emulator-libui PRIVATE Qt5::Network Qt5::WebEngineWidgets Qt5::WebChannel Qt5::WebSockets)
endif()

if(NOT LINUX_AARCH64)
  target_compile_options(emulator-libui PRIVATE "-DUSE_MMX=1" "-mmmx")
endif()

# Target specific compiler flags for windows, since we include FFMPEG C sources from C++ we need to make sure this flag is set for
# c++ sources.
android_target_compile_options(emulator-libui windows PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-Wno-literal-suffix>")

# Linux compiler settings
android_target_compile_options(
  emulator-libui linux-x86_64
  PRIVATE "-Wno-reserved-user-defined-literal" "-Wno-pointer-bool-conversion" "-Wno-deprecated-declarations"
          "-Wno-inconsistent-missing-override" "-Wno-return-type-c-linkage" "-Wno-invalid-constexpr")

# Mac Os compiler settings
android_target_compile_options(
  emulator-libui darwin-x86_64
  PRIVATE "-Wno-reserved-user-defined-literal" "-Wno-pointer-bool-conversion" "-Wno-deprecated-declarations"
          "-Wno-inconsistent-missing-override" "-Wno-return-type-c-linkage" "-Wno-invalid-constexpr")

# dependencies will remain internal, we should not be leaking out internal headers and defines.
target_link_libraries(
  emulator-libui
  PRIVATE android-emu
          emulator-libyuv
          FFMPEG::FFMPEG
          Qt5::Core
          Qt5::Widgets
          Qt5::Gui
          Qt5::Svg
          zlib
          android-hw-config)

# gl-widget.cpp needs to call XInitThreads() directly to work around a Qt bug. This implies a direct dependency to libX11.so
android_target_link_libraries(emulator-libui linux-x86_64 PRIVATE -lX11)
android_target_link_libraries(emulator-libui linux-aarch64 PRIVATE -lX11 -lxcb -lXau)
android_target_link_libraries(emulator-libui darwin-x86_64 PRIVATE "-framework Carbon")
# Windows-msvc specific dependencies. Need these for posix support.
android_target_link_libraries(emulator-libui windows_msvc-x86_64 PUBLIC dirent-win32)

list(APPEND android-libui-testdata testdata/mp4/video.mp4)

if(NOT LINUX_AARCH64)
  android_add_test(
    TARGET emulator-libui_unittests
    SRC # cmake-format: sortable
        android/mp4/MP4Dataset_test.cpp
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
        android/skin/rect_unittest.cpp)

  android_copy_test_files(emulator-libui_unittests "${android-libui-testdata}" testdata)

  target_compile_options(emulator-libui_unittests PRIVATE -O0 -UNDEBUG -Wno-deprecated-declarations)

  # Target specific compiler flags for windows
  android_target_compile_options(emulator-libui_unittests windows PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-Wno-literal-suffix>")

  # Linux compiler settings
  android_target_compile_options(emulator-libui_unittests linux-x86_64 PRIVATE "-Wno-reserved-user-defined-literal")

  # Mac Os compiler settings
  android_target_compile_options(emulator-libui_unittests darwin-x86_64 PRIVATE "-Wno-reserved-user-defined-literal")

  android_target_dependency(emulator-libui_unittests all QT5_SHARED_DEPENDENCIES)
  android_target_properties(emulator-libui_unittests all "${QT5_SHARED_PROPERTIES}")

  # Make sure we disable rtti in gtest
  target_compile_definitions(emulator-libui_unittests PRIVATE -DGTEST_HAS_RTTI=0)

  target_link_libraries(emulator-libui_unittests PRIVATE emulator-libui android-emu FFMPEG::FFMPEG gmock_main)

  android_target_link_libraries(emulator-libui_unittests windows_msvc-x86_64 PUBLIC dirent-win32)

endif()
# Version of libui without Qt
set(emulator-libui-headless_src ${ANDROID_LIBUI_HEADLESS_SRC_FILES})
android_add_library(
  TARGET emulator-libui-headless
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ${emulator-libui-headless_src}
  DARWIN ${emulator-libui-headless_darwin-x86_64_src}
  LINUX ${emulator-libui-headless_linux-x86_64_src}
  MSVC ${emulator-libui-headless_windows_msvc-x86_64_src})

target_compile_definitions(emulator-libui-headless PRIVATE -DCONFIG_HEADLESS)
if(NOT LINUX_AARCH64)
  target_compile_options(emulator-libui-headless PRIVATE "-DUSE_MMX=1" "-mmmx")
endif()
# Target specific compiler flags for windows, since we include FFMPEG C sources from C++ we need to make sure this flag is set for
# c++ sources.
if(NOT MSVC)
  android_target_compile_options(emulator-libui-headless windows PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-Wno-literal-suffix>")
endif()

# Linux compiler settings
android_target_compile_options(
  emulator-libui-headless linux-x86_64
  PRIVATE "-Wno-reserved-user-defined-literal" "-Wno-pointer-bool-conversion" "-Wno-deprecated-declarations"
          "-Wno-inconsistent-missing-override" "-Wno-return-type-c-linkage" "-Wno-invalid-constexpr")

# Mac Os compiler settings
android_target_compile_options(
  emulator-libui-headless darwin-x86_64
  PRIVATE "-Wno-reserved-user-defined-literal" "-Wno-pointer-bool-conversion" "-Wno-deprecated-declarations"
          "-Wno-inconsistent-missing-override" "-Wno-return-type-c-linkage" "-Wno-invalid-constexpr")

# dependencies will remain internal, we should not be leaking out internal headers and defines.
target_link_libraries(emulator-libui-headless PRIVATE android-emu emulator-libyuv FFMPEG::FFMPEG zlib android-hw-config)

if(WINDOWS_MSVC_X86_64)
  # Qt in windows will call main from win-main v.s. calling qt_main. we have to make a separate launch library to make sure that we
  # do not end up with duplicate main symbols when linking emulator-libui (it used to work do to a cmake linker quirk).
  android_add_library(TARGET emulator-winqt-launcher LICENSE Apache-2.0 SRC # cmake-format: sortable
                                                                            android/skin/qt/windows-qt-launcher.cpp)
endif()
