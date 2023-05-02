android_qt_lib(
  TARGET ext-page-snapshot
  INCLUDES src
  SRC android/skin/qt/extended-pages/snapshot-page.cpp
      android/skin/qt/extended-pages/snapshot-page.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)
