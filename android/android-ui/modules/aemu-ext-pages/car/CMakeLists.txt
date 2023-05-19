android_qt_lib(
  TARGET ext-page-car
  INCLUDES
  src
  SRC android/skin/qt/car-cluster-window.cpp
      android/skin/qt/extended-pages/car-cluster-connector/car-cluster-connector.cpp
      android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.cpp
      android/skin/qt/extended-pages/car-data-emulation/car-sensor-data.ui
      android/skin/qt/extended-pages/car-data-emulation/checkbox-dialog.cpp
      android/skin/qt/extended-pages/car-data-emulation/vhal-item.cpp
      android/skin/qt/extended-pages/car-data-emulation/vhal-table.cpp
      android/skin/qt/extended-pages/car-data-page.cpp
      android/skin/qt/extended-pages/car-data-page.ui
      android/skin/qt/extended-pages/car-rotary-page.cpp
      android/skin/qt/extended-pages/car-rotary-page.ui
      android/skin/qt/extended-pages/instr-cluster-render/car-cluster-widget.cpp
  DEPS aemu-ui-car-property-utils
       aemu-ui-qt-base
       android-emu
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets
       ui::common
       ui::gl-qt-bridge
       ui::widgets)

android_qt_lib(
  TARGET aemu-ui-car-property-utils
  INCLUDES  src
  SRC android/skin/qt/extended-pages/car-data-emulation/car-property-utils.cpp
  DEPS aemu-ui-qt-base
       android-emu-base
       android-emu-protos
       protobuf::libprotobuf
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets)
