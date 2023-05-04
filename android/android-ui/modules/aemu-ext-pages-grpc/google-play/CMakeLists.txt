android_qt_lib(
  TARGET ext-page-google-play-grpc
  INCLUDES src
  SRC android/skin/qt/extended-pages/google-play-page.cpp
      android/skin/qt/extended-pages/google-play-page.ui
  DEPS aemu-ui-qt-base
       android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::common
       ui::gl-qt-bridge
       ui::widgets)
