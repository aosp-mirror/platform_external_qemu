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
target_include_directories(GLcommon PUBLIC
    ${ANDROID_EMUGL_DIR}/host/libs/Translator/include
    ${ANDROID_EMUGL_DIR}/shared
    ${ANDROID_EMUGL_DIR}/host/include)
target_link_libraries(GLcommon PUBLIC android-emu-base astc-codec)
target_compile_options(GLcommon PRIVATE -fvisibility=hidden)
target_link_libraries(GLcommon PRIVATE emugl_base)
android_target_link_libraries(GLcommon linux-x86_64 PRIVATE "-ldl" "-Wl,-Bsymbolic")
android_target_link_libraries(GLcommon windows PRIVATE "gdi32::gdi32" "-Wl,--add-stdcall-alias")

set(GLcommon_unittests_src Etc2_unittest.cpp)
android_add_test(GLcommon_unittests)
target_link_libraries(GLcommon_unittests PUBLIC GLcommon gmock_main)
target_link_libraries(GLcommon_unittests PRIVATE emugl_base)
android_target_link_libraries(GLcommon_unittests linux-x86_64 PRIVATE "-ldl" "-Wl,-Bsymbolic")
android_target_link_libraries(GLcommon_unittests windows PRIVATE "gdi32::gdi32" "-Wl,--add-stdcall-alias")
