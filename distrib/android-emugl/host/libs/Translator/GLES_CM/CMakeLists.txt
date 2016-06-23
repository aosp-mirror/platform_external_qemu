
add_library(GLES_CM_translator SHARED
    GLEScmImp.cpp
    GLEScmUtils.cpp
    GLEScmContext.cpp
    GLEScmValidate.cpp
    )

target_link_libraries(GLES_CM_translator
    GLcommon
    )

