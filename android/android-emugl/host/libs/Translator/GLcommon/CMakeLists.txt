set(GLcommon_src
    etc.cpp
    FramebufferData.cpp
    GLBackgroundLoader.cpp
    GLDispatch.cpp
    GLutils.cpp
    GLEScontext.cpp
    GLESvalidate.cpp
    GLESpointer.cpp
    GLESbuffer.cpp
    NamedObject.cpp
    ObjectData.cpp
    ObjectNameSpace.cpp
    PaletteTexture.cpp
    RangeManip.cpp
    SaveableTexture.cpp
    ScopedGLState.cpp
    ShareGroup.cpp
    TextureData.cpp
    TextureUtils.cpp)
android_add_library(GLcommon)
android_target_include_directories(GLcommon all PUBLIC
    ${ANDROID_EMUGL_DIR}/host/libs/Translator/include
    ${ANDROID_EMUGL_DIR}/shared
    ${ANDROID_EMUGL_DIR}/host/include)
android_target_link_libraries(GLcommon all PUBLIC  android-emu-base emulator-astc-codec)
android_target_link_libraries(GLcommon all PRIVATE  emugl_base)
android_target_link_libraries(GLcommon linux-x86_64 PRIVATE "-ldl" "-Wl,-Bsymbolic")
android_target_link_libraries(GLcommon windows_PRIVATE "-lgdi32" "-Wl,--add-stdcall-alias")

set(lib64GLcommon_unittests_src Etc2_unittest.cpp)
android_add_test(lib64GLcommon_unittests)
android_target_link_libraries(lib64GLcommon_unittests all PUBLIC  GLcommon gmock_main)
android_target_link_libraries(lib64GLcommon_unittests all PRIVATE  emugl_base)
android_target_link_libraries(lib64GLcommon_unittests linux-x86_64 PRIVATE "-ldl" "-Wl,-Bsymbolic")
android_target_link_libraries(lib64GLcommon_unittests_libs_windows PRIVATE "-lgdi32" "-Wl,--add-stdcall-alias")
