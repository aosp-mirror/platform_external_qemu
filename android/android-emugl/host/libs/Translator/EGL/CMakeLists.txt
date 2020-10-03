android_add_library(
  TARGET EGL_translator_static
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ClientAPIExts.cpp
      EglConfig.cpp
      EglContext.cpp
      EglDisplay.cpp
      EglGlobalInfo.cpp
      EglImp.cpp
      EglPbufferSurface.cpp
      EglSurface.cpp
      EglThreadInfo.cpp
      EglValidate.cpp
      EglWindowSurface.cpp
      ThreadInfo.cpp
  DARWIN EglOsApi_darwin.cpp EglOsApi_egl.cpp MacNative.m
         MacPixelFormatsAttribs.m
  LINUX CoreProfileConfigs_linux.cpp EglOsApi_egl.cpp EglOsApi_glx.cpp
  WINDOWS CoreProfileConfigs_windows.cpp EglOsApi_egl.cpp EglOsApi_wgl.cpp)

target_link_libraries(EGL_translator_static PUBLIC GLcommon emugl_common)
target_link_libraries(EGL_translator_static PRIVATE emugl_base android-emu-base
                                                    astc-codec)
target_link_libraries(EGL_translator_static PUBLIC GLES_CM_translator_static
                                                   GLES_V2_translator_static)
target_compile_options(
  EGL_translator_static
  PRIVATE -fvisibility=hidden -Wno-inconsistent-missing-override
          -Wno-macro-redefined)
android_target_compile_options(EGL_translator_static darwin-x86_64
                               PRIVATE -Wno-deprecated-declarations)
android_target_compile_options(EGL_translator_static darwin-x86_64
                               PRIVATE "-Wno-deprecated-declarations")

# OS specific link dependencies
android_target_link_libraries(EGL_translator_static darwin-x86_64
                              PUBLIC "-framework AppKit")
android_target_link_libraries(EGL_translator_static linux-x86_64
                              PUBLIC "-lGL -lX11 -ldl -lpthread")
android_target_link_libraries(EGL_translator_static windows
                              PUBLIC "gdi32::gdi32")
