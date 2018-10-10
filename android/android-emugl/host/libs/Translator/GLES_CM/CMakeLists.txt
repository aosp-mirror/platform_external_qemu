set(GLES_CM_translator_src
    CoreProfileEngine.cpp
    GLEScmImp.cpp
    GLEScmUtils.cpp
    GLEScmContext.cpp
    GLEScmValidate.cpp)
android_add_shared_library(GLES_CM_translator)
# TODO: We are not consistent with our prefix naming..
set_target_properties(GLES_CM_translator PROPERTIES PREFIX "lib64")
target_compile_options(GLES_CM_translator PRIVATE -fvisibility=hidden)
target_link_libraries(GLES_CM_translator PUBLIC GLcommon android-emu emugl_common)
target_link_libraries(GLES_CM_translator PRIVATE emugl_base)
