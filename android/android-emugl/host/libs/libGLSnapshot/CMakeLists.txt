android_add_library(
  TARGET GLSnapshot LICENSE Apache-2.0 SRC # cmake-format: sortable
                                           GLSnapshot.cpp)
target_link_libraries(GLSnapshot PRIVATE emugl_base)
target_link_libraries(GLSnapshot PUBLIC GLESv2_dec OpenglCodecCommon
                                        emugl_common)
