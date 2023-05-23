android_qt_lib(
  TARGET ext-page-rotary-grpc
  INCLUDES src
  SRC android/skin/qt/extended-pages/rotary-input-dial.cpp
      android/skin/qt/extended-pages/rotary-input-page.cpp
      android/skin/qt/extended-pages/rotary-input-page.ui
  DEPS aemu-ui-qt-base
       android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Svg
       ui::gl-qt-bridge
       ui::widgets)
