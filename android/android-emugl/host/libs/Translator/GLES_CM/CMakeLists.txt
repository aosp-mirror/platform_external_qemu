set(GLES_CM_translator_src
    CoreProfileEngine.cpp
    GLEScmImp.cpp
    GLEScmUtils.cpp
    GLEScmContext.cpp
    GLEScmValidate.cpp)
android_add_shared_library(GLES_CM_translator)
target_compile_options(GLES_CM_translator PRIVATE -fvisibility=hidden)
target_link_libraries(GLES_CM_translator PUBLIC GLcommon android-emu emugl_common)
target_link_libraries(GLES_CM_translator PRIVATE emugl_base)
android_install_shared(GLES_CM_translator)