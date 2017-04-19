set(SOURCES
  etc1.cpp
  log.cpp
  RefBase.cpp
  )

add_library(GLES12TranslatorCommon ${SOURCES})
target_link_libraries(GLES12TranslatorCommon emugl_common)
