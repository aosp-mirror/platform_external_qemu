# Don't treat any warnings as error as we take the source directly from
# upstream and just compile it.
project(ANDROID_EMUGL)


include_directories(
  ${PREBUILTS}/android-emulator-build/common/ANGLE/${BUILD_HOST_TAG64}/include
  ${ANDROID_ROOT}/android-emu
  ${ANDROID_ROOT}/android-emugl/shared
  ${ANDROID_ROOT}/android-emugl/host/include
  ${ANDROID_ROOT}/android-emugl/shared/OpenglCodecCommon
  ${ANDROID_ROOT}/android-emugl/host/libs
  ${ANDROID_ROOT}/android-emugl/host/include/libOpenglRender
  ${ANDROID_ROOT}/android-emugl/host/include/OpenGLESDispatch
  ${ANDROID_ROOT}/android-emugl/host/libs/GLESv1_dec
  ${ANDROID_ROOT}/android-emugl/host/libs/GLESv2_dec
  ${ANDROID_ROOT}/android-emugl/host/libs/libGLSnapshot
  ${ANDROID_ROOT}/android-emugl/host/libs/renderControl_dec
  ${ANDROID_ROOT}/android-emugl/host/libs/Translator/
  ${ANDROID_ROOT}/android-emugl/host/libs/Translator/include
  )

add_subdirectory(host)
add_subdirectory(shared)
