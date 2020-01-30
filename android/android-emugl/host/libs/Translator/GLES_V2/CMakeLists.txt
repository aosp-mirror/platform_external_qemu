prebuilt(ANGLE)
android_add_library(
  TARGET GLES_V2_translator
  LICENSE Apache-2.0
  SHARED
  SRC # cmake-format: sortable
      GLESv2Imp.cpp
      GLESv2Context.cpp
      GLESv2Validate.cpp
      SamplerData.cpp
      ShaderParser.cpp
      ShaderValidator.cpp
      TransformFeedbackData.cpp
      ProgramData.cpp
      ANGLEShaderParser.cpp)
target_compile_options(GLES_V2_translator PRIVATE -fvisibility=hidden)
target_link_libraries(
  GLES_V2_translator PRIVATE GLcommon OpenglCodecCommon emugl_common
                             android-emu ANGLE::ANGLE)
target_link_libraries(GLES_V2_translator PRIVATE emugl_base)
android_install_shared(GLES_V2_translator)
