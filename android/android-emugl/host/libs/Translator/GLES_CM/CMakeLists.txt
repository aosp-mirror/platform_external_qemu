set(GLES_CM_translator_src
    CoreProfileEngine.cpp
    GLEScmImp.cpp
    GLEScmUtils.cpp
    GLEScmContext.cpp
    GLEScmValidate.cpp)
android_add_shared_library(GLES_CM_translator)
# TODO: We are not consistent with our prefix naming..
set_target_properties(GLES_CM_translator PROPERTIES PREFIX "lib64")
android_target_link_libraries(GLES_CM_translator all PUBLIC GLcommon android-emu emugl_common)
android_target_link_libraries(GLES_CM_translator all PRIVATE emugl_base)
