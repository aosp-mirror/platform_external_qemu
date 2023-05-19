android_qt_lib(
  TARGET ext-page-help
  INCLUDES src
  SRC android/skin/qt/extended-pages/help-page.cpp
      android/skin/qt/extended-pages/help-page.ui
  DEPS aemu-ui-qt-base
       android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::gl-qt-bridge
       ui::widgets)
