include_directories(${GLESv1_dec_INCLUDE_DIRS})
include_directories(${GLESv2_dec_INCLUDE_DIRS})
list(APPEND SOURCES
	#BufferQueue_unittest.cpp
	ChannelStream.cpp
	ColorBuffer.cpp
	FbConfig.cpp
	FenceSync.cpp
	FrameBuffer.cpp
	GLESVersionDetector.cpp
	OpenGLTestContext.cpp
	#OpenGL_unittest.cpp
	ReadBuffer.cpp
	RenderChannelImpl.cpp
	RenderContext.cpp
	RenderControl.cpp
	RenderLibImpl.cpp
	RenderThread.cpp
	RenderThreadInfo.cpp
	RenderWindow.cpp
	RendererImpl.cpp
	#StalePtrRegistry_unittest.cpp
	SyncThread.cpp
	TextureDraw.cpp
	TextureResize.cpp
	WindowSurface.cpp
	YUVConverter.cpp
	render_api.cpp
	)


if(APPLE)
  list(APPEND SOURCES 
	  )
elseif(UNIX)
  list(APPEND SOURCES 
	NativeSubWindow_x11.cpp
	  )
elseif(WIN32)
  list(APPEND SOURCES 
	NativeSubWindow_win32.cpp
	  )
endif()

include_directories(${GLESv2_dec_INCLUDE_DIRS}
	${GLESv1_dec_INCLUDE_DIRS}
	${renderControl_dec_INCLUDE_DIRS}
	)
add_library(OpenglRender ${SOURCES})

