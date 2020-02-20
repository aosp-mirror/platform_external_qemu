android_add_library(
  TARGET GLES_CM_translator
  SHARED
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      CoreProfileEngine.cpp GLEScmImp.cpp GLEScmUtils.cpp GLEScmContext.cpp
      GLEScmValidate.cpp)
target_compile_options(GLES_CM_translator PRIVATE -fvisibility=hidden)
if (OPTION_GFXSTREAM_BACKEND)
  target_link_libraries(GLES_CM_translator PUBLIC GLcommon android-emu-shared
                                                  emugl_common)
else()
  target_link_libraries(GLES_CM_translator PUBLIC GLcommon android-emu
                                                  emugl_common)
endif()
target_link_libraries(GLES_CM_translator PRIVATE emugl_base)
android_install_shared(GLES_CM_translator)
