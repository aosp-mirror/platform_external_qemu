android_qt_lib(
  TARGET ext-page-camera-grpc
  INCLUDES
  src
  SRC android/skin/qt/extended-pages/camera-page-grpc.cpp
      android/skin/qt/extended-pages/camera-page-grpc.ui
      android/skin/qt/extended-pages/camera-virtualscene-subpage-grpc.cpp
      android/skin/qt/extended-pages/camera-virtualscene-subpage-grpc.ui
  DEPS aemu-ui-qt-base
       android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Svg
       ui::gl-qt-bridge
       ui::widgets)

target_link_libraries(ext-page-camera-grpc PUBLIC virtual-scene-service-client)
