
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(OPENGL_RENDER_PLATFORM_SRCS NativeSubWindow_x11.cpp)
    set(OPENGL_RENDER_PLATFORM_LIBS -lX11 -lrt)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(OPENGL_RENDER_PLATFORM_SRCS NativeSubWindow_cocoa.m)
    set(OPENGL_RENDER_PLATFORM_LIBS -Wl,-framework,AppKit)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(OPENGL_RENDER_PLATFORM_SRCS NativeSubWindow_win32.cpp)
    set(OPENGL_RENDER_PLATFORM_LIBS -lgdi32)
endif()

add_library(OpenglRender SHARED
    ${OPENGL_RENDER_PLATFORM_SRCS}
    ChannelStream.cpp
    ColorBuffer.cpp
    FbConfig.cpp
    FrameBuffer.cpp
    ReadBuffer.cpp
    RenderChannelImpl.cpp
    RenderContext.cpp
    RenderControl.cpp
    RendererImpl.cpp
    RenderLibImpl.cpp
    RenderThread.cpp
    RenderThreadInfo.cpp
    render_api.cpp
    RenderWindow.cpp
    TextureDraw.cpp
    TextureResize.cpp
    TimeUtils.cpp
    WindowSurface.cpp
    )

target_link_libraries(OpenglRender
    PRIVATE
    ${OPENGL_RENDER_PLATFORM_LIBS}
    renderControl_dec
    OpenglCodecCommon
    OpenGLESDispatch
    android-emu-base
    )

target_include_directories(OpenglRender
    PRIVATE
    ../libOpenGLESDispatch
    )

