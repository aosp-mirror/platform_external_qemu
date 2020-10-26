android_add_library(
  TARGET GLcommon
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      etc.cpp
      FramebufferData.cpp
      GLBackgroundLoader.cpp
      GLDispatch.cpp
      GLESbuffer.cpp
      GLEScontext.cpp
      GLESpointer.cpp
      GLESvalidate.cpp
      GLutils.cpp
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
target_include_directories(
  GLcommon PUBLIC ${ANDROID_EMUGL_DIR}/host/libs/Translator/include
                  ${ANDROID_EMUGL_DIR}/shared ${ANDROID_EMUGL_DIR}/host/include)
target_link_libraries(GLcommon PUBLIC android-emu-base astc-codec)
target_compile_options(GLcommon PRIVATE -fvisibility=hidden)
target_compile_options(GLcommon PUBLIC -Wno-inconsistent-missing-override)
target_link_libraries(GLcommon PRIVATE emugl_base)
android_target_link_libraries(GLcommon linux-x86_64 PRIVATE "-ldl"
                                                            "-Wl,-Bsymbolic")
android_target_link_libraries(GLcommon windows
                              PRIVATE "gdi32::gdi32" "-Wl,--add-stdcall-alias")

android_add_test(TARGET GLcommon_unittests SRC # cmake-format: sortable
                                               Etc2_unittest.cpp)
target_link_libraries(GLcommon_unittests PUBLIC GLcommon gmock_main)
target_link_libraries(GLcommon_unittests PRIVATE emugl_base)
android_target_link_libraries(GLcommon_unittests linux-x86_64
                              PRIVATE "-ldl" "-Wl,-Bsymbolic")
android_target_link_libraries(GLcommon_unittests windows
                              PRIVATE "gdi32::gdi32" "-Wl,--add-stdcall-alias")
