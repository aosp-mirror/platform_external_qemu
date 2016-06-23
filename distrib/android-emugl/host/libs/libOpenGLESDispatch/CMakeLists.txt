
add_library(OpenGLESDispatch STATIC
    EGLDispatch.cpp
    GLESv2Dispatch.cpp
    GLESv1Dispatch.cpp
    )

#target_include_directories(OpenGLESDispatch PRIVATE ../Translator/include)
target_link_libraries(OpenGLESDispatch
    PUBLIC
    GLESv2_dec
    GLESv1_dec
    emugl_common
    )

target_include_directories(OpenGLESDispatch
    PUBLIC
    ../Translator/include
    )

