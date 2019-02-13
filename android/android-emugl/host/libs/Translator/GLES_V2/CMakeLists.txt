prebuilt(ANGLE)
set(GLES_V2_translator_src
    GLESv2Imp.cpp
    GLESv2Context.cpp
    GLESv2Validate.cpp
    SamplerData.cpp
    ShaderParser.cpp
    ShaderValidator.cpp
    TransformFeedbackData.cpp
    ProgramData.cpp
    ANGLEShaderParser.cpp)
android_add_shared_library(GLES_V2_translator)
# Set fvisibility as default to export all symbol names for debugging purpose.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(GLES_V2_translator PRIVATE -fvisibility=default)
else()
  target_compile_options(GLES_V2_translator PRIVATE -fvisibility=hidden)
endif()
target_link_libraries(GLES_V2_translator
                              PRIVATE
                              GLcommon
                              OpenglCodecCommon
                              emugl_common
                              android-emu
                              ANGLE::ANGLE)
target_link_libraries(GLES_V2_translator PRIVATE emugl_base)
android_install_shared(GLES_V2_translator)
