android_qt_lib(
  TARGET ext-page-finger-grpc
  INCLUDES src
  SRC android/skin/qt/extended-pages/finger-page-grpc.cpp
      android/skin/qt/extended-pages/finger-page-grpc.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)

target_link_libraries(ext-page-finger-grpc
                      PUBLIC emulator_controller-service-client)
