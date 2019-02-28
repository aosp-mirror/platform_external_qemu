set(EGL_translator_src
    ThreadInfo.cpp
    EglImp.cpp
    EglConfig.cpp
    EglContext.cpp
    EglGlobalInfo.cpp
    EglValidate.cpp
    EglSurface.cpp
    EglWindowSurface.cpp
    EglPbufferSurface.cpp
    EglThreadInfo.cpp
    EglDisplay.cpp
    ClientAPIExts.cpp)

set(EGL_translator_darwin-x86_64_src EglOsApi_darwin.cpp EglOsApi_egl.cpp MacNative.m MacPixelFormatsAttribs.m)
set(EGL_translator_linux-x86_64_src EglOsApi_glx.cpp EglOsApi_egl.cpp CoreProfileConfigs_linux.cpp)
set(EGL_translator_windows_src EglOsApi_wgl.cpp EglOsApi_egl.cpp CoreProfileConfigs_windows.cpp)

android_add_shared_library(EGL_translator)

target_link_libraries(EGL_translator PUBLIC GLcommon emugl_common)
target_link_libraries(EGL_translator PRIVATE emugl_base android-emu-base astc-codec)
target_compile_options(EGL_translator PRIVATE -fvisibility=hidden)

# OS specific link dependencies
android_target_link_libraries(EGL_translator darwin-x86_64 PUBLIC "-framework AppKit")
android_target_link_libraries(EGL_translator linux-x86_64 PUBLIC "-lGL -lX11 -ldl -lpthread")
android_target_link_libraries(EGL_translator windows PUBLIC "gdi32::gdi32")
android_install_shared(EGL_translator)
