android_qt_lib(
  TARGET ext-page-battery-grpc INCUDES  src
  SRC android/skin/qt/extended-pages/battery-page-grpc.cpp
      android/skin/qt/extended-pages/battery-page-grpc.ui
  INCLUDES  src
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)
target_link_libraries(ext-page-battery-grpc
                      PUBLIC emulator_controller-service-client)
