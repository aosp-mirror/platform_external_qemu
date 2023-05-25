android_qt_lib(
  TARGET ext-page-settings-grpc
  INCLUDES src
  SRC android/skin/qt/extended-pages/perfstats-page.cpp
      android/skin/qt/extended-pages/perfstats-page.ui
      android/skin/qt/extended-pages/settings-page-proxy.cpp
      android/skin/qt/extended-pages/settings-page.cpp
      android/skin/qt/extended-pages/settings-page.ui
      android/skin/qt/perf-stats-3d-widget.cpp
  DEPS aemu-ui-qt-base
       android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::gl-qt-bridge
       ui::widgets)
