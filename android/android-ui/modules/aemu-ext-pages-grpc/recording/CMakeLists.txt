android_qt_lib(
  TARGET ext-page-recording-grpc
  INCLUDES src
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
  DEPS aemu-ui-qt-base
       aemu::recording
       android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::gl-qt-bridge
       ui::widgets)
