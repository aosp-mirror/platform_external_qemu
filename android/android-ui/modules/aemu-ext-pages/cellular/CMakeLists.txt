
android_qt_lib(
  TARGET ext-page-cellular
  INCLUDES src
  SRC android/skin/qt/extended-pages/cellular-page.cpp
      android/skin/qt/extended-pages/cellular-page.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)