prebuilt(ANGLE)
set(GLES_V2_translator_src
    GLESv2Imp.cpp
    GLESv2Context.cpp
    GLESv2Validate.cpp
    SamplerData.cpp
    ShaderParser.cpp
    ShaderValidator.cpp
    ProgramData.cpp
    ANGLEShaderParser.cpp)
android_add_shared_library(GLES_V2_translator)
target_compile_options(GLES_V2_translator PRIVATE -fvisibility=hidden)
target_link_libraries(GLES_V2_translator
                              PUBLIC
                              GLcommon
                              OpenglCodecCommon
                              emugl_common
                              android-emu
                              ${ANGLE_LIBRARIES})
target_link_libraries(GLES_V2_translator PRIVATE emugl_base)
target_include_directories(GLES_V2_translator PRIVATE ${ANGLE_INCLUDE_DIRS})
# TODO: We are not consistent with our prefix naming..
set_target_properties(GLES_V2_translator PROPERTIES PREFIX "lib64")
