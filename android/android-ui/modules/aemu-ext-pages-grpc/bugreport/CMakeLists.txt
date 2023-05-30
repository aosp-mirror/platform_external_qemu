android_qt_lib(
  TARGET ext-page-bugreport-grpc
  INCLUDES
  src
  SRC android/skin/qt/extended-pages/bug-report-page-grpc.cpp
      android/skin/qt/extended-pages/bug-report-page-grpc.ui
  DEPS android-emu-adb-interface
       android-emu-avd
       android-emu-files
       android-emu-metrics
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::common
       ui::gl-qt-bridge
       ui::widgets)

target_link_libraries(ext-page-bugreport-grpc
                      PUBLIC emulator_controller-service-client)
