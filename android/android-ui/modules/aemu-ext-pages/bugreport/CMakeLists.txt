android_qt_lib(
  TARGET ext-page-bugreport
  INCLUDES src
  SRC android/skin/qt/extended-pages/bug-report-page.cpp
      android/skin/qt/extended-pages/bug-report-page.ui
  DEPS android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::common
       ui::gl-qt-bridge
       ui::widgets)
