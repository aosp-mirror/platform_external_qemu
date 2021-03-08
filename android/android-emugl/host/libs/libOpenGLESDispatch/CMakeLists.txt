android_add_library(
  TARGET OpenGLESDispatch
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      EGLDispatch.cpp
      gles1_stubbed_in_translator_namespace.cpp
      gles2_stubbed_in_translator_namespace.cpp
      GLESv1Dispatch.cpp
      GLESv2Dispatch.cpp
      OpenGLDispatchLoader.cpp
      StaticDispatch.cpp)
target_compile_options(OpenGLESDispatch PRIVATE -fvisibility=hidden)
target_link_libraries(OpenGLESDispatch PRIVATE emugl_base)
target_link_libraries(OpenGLESDispatch PUBLIC GLESv2_dec OpenglCodecCommon
                                              emugl_common GLESv1_dec)
target_link_libraries(OpenGLESDispatch PRIVATE EGL_translator_static)
