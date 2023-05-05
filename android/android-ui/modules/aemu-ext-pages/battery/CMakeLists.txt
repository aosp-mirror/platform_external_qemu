android_qt_lib(
  TARGET ext-page-battery
  INCUDES src
  SRC android/skin/qt/extended-pages/battery-page.cpp
      android/skin/qt/extended-pages/battery-page.ui
  INCLUDES src
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)
