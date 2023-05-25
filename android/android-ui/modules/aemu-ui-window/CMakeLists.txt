android_qt_lib(
  TARGET aemu-ui-window
  ALIAS ui::window
  SRC android/emulator-window.c
      android/main-common-ui.c
      android/resource.c
      android/skin/qt/emulator-no-qt-no-window.cpp
      android/skin/ui.c
      android/skin/user-config.c
      android/skin/window.c
  DEPS aemu-recording
       android-emu-adb-interface
       android-emu-agents
       android-emu-avd
       android-emu-base
       android-emu-base-headers
       android-emu-cmdline
       android-emu-feature
       android-emu-files
       android-emu-hardware
       android-emu-metrics
       android-emu-sockets
       android-emu-telephony
       android-emu-utils
       android-emu-net
       android-hw-config
       qemu-host-common-headers
       aemu-gl-init
       ui::common)
      #  aemu-gl-init)
target_include_directories(
  aemu-ui-window
  PRIVATE
    src
    # Fix this dependency once those libs have been sliced.
    ${ANDROID_QEMU2_TOP_DIR}/android/android-emu
    # use ui::gl-bridge once the cycles have been broken.
    # ../aemu-gl-bridge/include
    # TODO(jansene): We actually have a hard dependency on qemu-glue as there
    # are a lot of externs that are actually defined in qemu2-glue. this has to
    # be sorted out,
    ${ANDROID_QEMU2_TOP_DIR}/android-qemu2-glue/config/${ANDROID_TARGET_TAG}
  PUBLIC include)
