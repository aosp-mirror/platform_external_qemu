
add_library(GLES_V2_translator SHARED
    GLESv2Imp.cpp
    GLESv2Context.cpp
    GLESv2Validate.cpp
    ShaderParser.cpp
    ShaderValidator.cpp
    ProgramData.cpp
    )

target_link_libraries(GLES_V2_translator
    GLcommon
    )
