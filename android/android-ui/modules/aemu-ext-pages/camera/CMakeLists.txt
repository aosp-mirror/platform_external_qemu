android_qt_lib(
  TARGET ext-page-camera
  INCLUDES src
  SRC android/skin/qt/device-3d-widget.cpp
      android/skin/qt/extended-pages/camera-page.cpp
      android/skin/qt/extended-pages/camera-page.ui
      android/skin/qt/extended-pages/camera-virtualscene-subpage.cpp
      android/skin/qt/extended-pages/camera-virtualscene-subpage.ui
      android/skin/qt/extended-pages/virtual-sensors-page.cpp
      android/skin/qt/extended-pages/virtual-sensors-page.ui
  DEPS aemu-ui-qt-base
       android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Svg
       ui::gl-qt-bridge
       ui::widgets)
