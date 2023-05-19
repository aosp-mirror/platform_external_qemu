android_qt_lib(
  TARGET aemu-ui-widgets
  # SHARED
  ALIAS ui::widgets
  SRC android/skin/qt/angle-input-widget.cpp
      android/skin/qt/angle-input-widget.h
      android/skin/qt/common-controls/
      android/skin/qt/common-controls/cc-list-item.cpp
      android/skin/qt/common-controls/cc-list-item.h
      android/skin/qt/common.cpp
      android/skin/qt/editable-slider-widget.cpp
      android/skin/qt/editable-slider-widget.h
      android/skin/qt/error-dialog.cpp
      android/skin/qt/event-capturer.cpp
      android/skin/qt/event-serializer.cpp
      android/skin/qt/event-subscriber.cpp
      android/skin/qt/poster-image-well.cpp
      android/skin/qt/poster-image-well.h
      android/skin/qt/poster-image-well.ui
      android/skin/qt/qt-ui-commands.cpp
      android/skin/qt/QtThreading.cpp
      android/skin/qt/raised-material-button.h
      android/skin/qt/size-tweaker.cpp
      android/skin/qt/ui-event-recorder.cpp
      android/skin/qt/user-actions-counter.cpp
      android/skin/qt/wavefront-obj-parser.cpp
  DEPS aemu-recording
       android-emu
       android-emu-base
       qemu-host-common-headers
       Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Gui
       Qt${QT_VERSION_MAJOR}::Widgets
       webrtc-yuv)

set_target_properties(aemu-ui-widgets PROPERTIES AUTOMOC TRUE)
target_include_directories(
  aemu-ui-widgets
  PUBLIC src
  PRIVATE include/android/skin/qt/
          include/android/skin/qt/common-controls
          include
          # We need access to the settings agent.
          ${ANDROID_QEMU2_TOP_DIR}/android/android-emu)
