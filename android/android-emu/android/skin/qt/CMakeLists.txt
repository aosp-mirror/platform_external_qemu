list(APPEND SOURCES
  #  accelerometer-3d-widget.cpp
  #  angle-input-widget.cpp
  #  editable-slider-widget.cpp
  #  emulator-container.cpp
  #  emulator-overlay.cpp
  #  emulator-qt-no-window.cpp
  #  emulator-qt-window.cpp
  #  error-dialog.cpp
  #  event-capturer.cpp
  #  event-qt.cpp
  #  event-serializer.cpp
  #  event-subscriber.cpp
  #  extended-window.cpp
  #  gl-canvas.cpp
    gl-common.cpp
  #  gl-texture-draw.cpp
  #  gl-widget.cpp
  #  init-qt.cpp
  #  QtLooper.cpp
  #  qt-ui-commands.cpp
  #  size-tweaker.cpp
  #  stylesheet.cpp
  #  surface-qt.cpp
  #  tool-window.cpp
  #  ui-event-recorder.cpp
  #  user-actions-counter.cpp
  #  wavefront-obj-parser.cpp
  #  windows-native-window.cpp
  #  winsys-qt.cpp
  )

include_directories(${CONFIG_HOST_INCLUDE_DIRS})
add_library(skin_qt ${SOURCES})
