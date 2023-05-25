android_qt_lib(
  TARGET ext-page-snapshot-grpc
  INCLUDES src
  SRC android/skin/qt/extended-pages/snapshot-page-grpc.cpp
      android/skin/qt/extended-pages/snapshot-page-grpc.ui
  DEPS absl::statusor
       android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::gl-qt-bridge
       ui::widgets)
target_link_libraries(ext-page-snapshot-grpc PUBLIC snapshot-service-client)
