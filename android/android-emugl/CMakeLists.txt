# Don't treat any warnings as error as we take the source directly from
# upstream and just compile it.
project(ANDROID_EMUGL)
set(CMAKE_C_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "-std=c++11 -Wall")

set(ANDROID_ROOT ${ANDROID_EMUGL_SOURCE_DIR}/..)

# Ensure -fPIC
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(
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
