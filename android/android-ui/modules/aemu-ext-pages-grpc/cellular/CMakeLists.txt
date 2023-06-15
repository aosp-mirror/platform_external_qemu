android_qt_lib(
  TARGET ext-page-cellular-grpc
  INCLUDES src
  SRC android/skin/qt/extended-pages/cellular-page-grpc.cpp
      android/skin/qt/extended-pages/cellular-page-grpc.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)

target_link_libraries(ext-page-cellular-grpc PUBLIC modem-service-client)
