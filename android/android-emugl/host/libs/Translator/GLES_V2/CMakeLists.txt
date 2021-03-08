prebuilt(ANGLE)

android_add_library(
  TARGET GLES_V2_translator_static
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ANGLEShaderParser.cpp
      GLESv2Context.cpp
      GLESv2Imp.cpp
      GLESv2Validate.cpp
      ProgramData.cpp
      SamplerData.cpp
      ShaderParser.cpp
      ShaderValidator.cpp
      TransformFeedbackData.cpp)
target_compile_options(GLES_V2_translator_static PRIVATE -fvisibility=hidden
                                                         -Wno-macro-redefined)
if(OPTION_GFXSTREAM_BACKEND)
  target_link_libraries(
    GLES_V2_translator_static PRIVATE GLcommon OpenglCodecCommon emugl_common
                                      android-emu-shared ANGLE::ANGLE)
else()
  target_link_libraries(
    GLES_V2_translator_static PRIVATE GLcommon OpenglCodecCommon emugl_common
                                      android-emu ANGLE::ANGLE)
endif()
target_link_libraries(GLES_V2_translator_static PRIVATE emugl_base)
