android_qt_lib(
  TARGET ext-page-telephony
  INCLUDES src
  SRC android/skin/qt/extended-pages/telephony-page.cpp
      android/skin/qt/extended-pages/telephony-page.ui
  DEPS android-emu qemu-host-common-headers Qt${QT_VERSION_MAJOR}::Core
       Qt${QT_VERSION_MAJOR}::Widgets ui::gl-qt-bridge ui::widgets)