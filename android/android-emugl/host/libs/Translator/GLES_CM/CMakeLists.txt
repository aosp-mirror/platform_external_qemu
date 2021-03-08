android_add_library(
  TARGET GLES_CM_translator_static
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      CoreProfileEngine.cpp GLEScmContext.cpp GLEScmImp.cpp GLEScmUtils.cpp
      GLEScmValidate.cpp)
target_compile_options(GLES_CM_translator_static PRIVATE -fvisibility=hidden
                                                         -Wno-macro-redefined)

if(OPTION_GFXSTREAM_BACKEND)
  target_link_libraries(GLES_CM_translator_static
                        PUBLIC GLcommon android-emu-shared emugl_common)
else()
  target_link_libraries(GLES_CM_translator_static PUBLIC GLcommon android-emu
                                                         emugl_common)
endif()
target_link_libraries(GLES_CM_translator_static PRIVATE emugl_base)
