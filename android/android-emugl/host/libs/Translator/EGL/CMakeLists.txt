list(APPEND SOURCES
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


if(APPLE)
  list(APPEND SOURCES EglOsApi_darwin
    MacNative.m
    MacPixelFormatsAttribs.m)
  set(EGL_LDFLAGS -Wl,-framework,AppKit)
elseif(UNIX)
  list(APPEND SOURCES EglOsApi_glx.cpp)
elseif(WIN32)
  list(APPEND SOURCES EgoOsApi_wgl.cpp)
endif()

include_directories(BEFORE
  ${EGL_INCLUDE_DIRS}
  ${GLESv2_INCLUDE_DIRS})

add_library(EGL_translator SHARED ${SOURCES})
target_link_libraries(EGL_translator GLcommon ${EGL_LDFLAGS} ${EGL_LIBRARIES})
