android_qt_lib(
  TARGET ext-page-display
  INCLUDES src
  SRC android/skin/qt/extended-pages/multi-display-arrangement.cpp
      android/skin/qt/extended-pages/multi-display-item.cpp
      android/skin/qt/extended-pages/multi-display-page.cpp
      android/skin/qt/extended-pages/multi-display-page.ui
  DEPS aemu-ui-qt-base
       android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::gl-qt-bridge
       ui::widgets)
