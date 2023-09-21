android_qt_lib(
  TARGET ext-page-telephony-grpc
  INCLUDES
  src
  SRC android/skin/qt/extended-pages/telephony-page-grpc.cpp
      android/skin/qt/extended-pages/telephony-page-grpc.ui
  DEPS android-emu
       modem-service-client
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::gl-qt-bridge
       ui::widgets)
target_compile_options(ext-page-telephony-grpc PRIVATE -Wno-switch)
