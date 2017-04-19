set(SOURCES
  # Etc2_unittest.cpp
  GLEScontext.cpp
  ObjectData.cpp
  ShareGroup.cpp
  etc.cpp
  GLESpointer.cpp
  ObjectNameSpace.cpp
  TextureData.cpp
  FramebufferData.cpp
  GLESvalidate.cpp
  PaletteTexture.cpp
  TextureUtils.cpp
  GLDispatch.cpp
  GLutils.cpp
  RangeManip.cpp
  GLESbuffer.cpp
  NamedObject.cpp
  SaveableTexture.cpp
  )

add_library(GLcommon ${SOURCES})
target_link_libraries(GLcommon emugl_common ANDROID_BASE_LIBRARIES ${EXTRA_LIBS})
