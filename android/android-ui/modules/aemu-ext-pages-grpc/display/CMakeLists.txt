android_qt_lib(
  TARGET ext-page-display-grpc
  INCLUDES src
  SRC android/skin/qt/extended-pages/multi-display-arrangement-grpc.cpp
      android/skin/qt/extended-pages/multi-display-item-grpc.cpp
      android/skin/qt/extended-pages/multi-display-page-grpc.cpp
      android/skin/qt/extended-pages/multi-display-page-grpc.ui
  DEPS aemu-ui-qt-base
       android-emu
       emulator_controller-service-client
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::gl-qt-bridge
       ui::widgets)
