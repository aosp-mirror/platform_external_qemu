android_add_library(
  TARGET OpenglCodecCommon LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ChecksumCalculator.cpp ChecksumCalculatorThreadInfo.cpp glUtils.cpp)

target_compile_options(OpenglCodecCommon PRIVATE -fvisibility=hidden)
target_link_libraries(OpenglCodecCommon PRIVATE emugl_base)
target_link_libraries(OpenglCodecCommon PUBLIC emugl_common android-emu)
target_include_directories(
  OpenglCodecCommon
  PRIVATE ${ANDROID_EMUGL_DIR}/host/libs/Translator/include
          ${ANDROID_EMUGL_DIR}/shared ${ANDROID_EMUGL_DIR}/host/include)
