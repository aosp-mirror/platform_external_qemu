
android_qt_lib(
  TARGET ext-page-sensor-replay
  INCLUDES src
  SRC android/skin/qt/extended-pages/sensor-replay-item.cpp
      android/skin/qt/extended-pages/sensor-replay-page.cpp
      android/skin/qt/extended-pages/sensor-replay-page.ui
  DEPS aemu-ui-car-property-utils
       android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::common
       ui::gl-qt-bridge
       ui::widgets)
