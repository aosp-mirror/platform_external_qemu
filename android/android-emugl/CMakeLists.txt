cmake_minimum_required(VERSION 2.8.11)
project(ANDROID_EMU)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/../cmake/")

include(AddCXXCompilerFlag)
include(AddGlobalVariables)

# Call this first to setup various dependencies
add_global_variables()

include_directories(
  ${ANDROID_EMU}
  ${ANDROID_EMUGL}/shared
  ${ANDROID_EMUGL}/host/include
  ${ANDROID_EMUGL}/shared/OpenglCodecCommon
  ${ANDROID_EMUGL}/host/libs
  ${ANDROID_EMUGL}/host/include/libOpenglRender
  ${ANDROID_EMUGL}/host/include/OpenGLESDispatch
  ${ANDROID_EMUGL}/host/libs/GLESv1_dec
  ${ANDROID_EMUGL}/host/libs/GLESv2_dec
  ${ANDROID_EMUGL}/host/libs/libGLSnapshot
  ${ANDROID_EMUGL}/host/libs/renderControl_dec
  ${ANDROID_EMUGL}/host/libs/Translator/
  ${ANDROID_EMUGL}/host/libs/Translator/include
  )

add_cxx_compiler_flag(-Wall)
add_cxx_compiler_flag(-std=c++1y)
add_cxx_compiler_flag(-fPIC)
add_cxx_compiler_flag(-w)
add_definitions(-DEMUGL_BUILD=1)

if (APPLE)
  add_cxx_compiler_flag(-fvisibility=hidden)
elseif(LINUX)
  add_cxx_compiler_flag(-fvisibility=internal)
endif()

add_subdirectory(${ANDROID_EMU}/android/base base)
add_subdirectory(host)
add_subdirectory(shared)
