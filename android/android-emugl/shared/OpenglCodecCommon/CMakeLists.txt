set(OpenglCodecCommon_src glUtils.cpp ChecksumCalculator.cpp ChecksumCalculatorThreadInfo.cpp)
android_add_library(OpenglCodecCommon)

android_target_link_libraries(OpenglCodecCommon all PRIVATE emugl_base)
android_target_link_libraries(OpenglCodecCommon all PUBLIC emugl_common android-emu-base)

android_target_include_directories(OpenglCodecCommon
                                   all
                                   PRIVATE
                                   ${ANDROID_EMUGL_DIR}/host/libs/Translator/include
                                   ${ANDROID_EMUGL_DIR}/shared
                                   ${ANDROID_EMUGL_DIR}/host/include)
