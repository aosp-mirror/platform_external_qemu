prebuilt (ANGLE)

set (host_common_SRC_FILES
  ChannelStream.cpp
  ColorBuffer.cpp
  FbConfig.cpp
  FenceSync.cpp
  FrameBuffer.cpp
  GLESVersionDetector.cpp
  PostWorker.cpp
  ReadbackWorker.cpp
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
  SyncThread.cpp
  TextureDraw.cpp
  TextureResize.cpp
  WindowSurface.cpp
  YUVConverter.cpp
)

if(APPLE OR ${ANDROID_TARGET_TAG} STREQUAL "darwin-x86_64")
    list(APPEND host_common_SRC_FILES
    NativeSubWindow_cocoa.m
    )
  add_library (
    libOpenglRender
    SHARED
    ${host_common_SRC_FILES}
  )
  target_link_libraries (libOpenglRender PUBLIC "-framework AppKit")
  target_compile_options (libOpenglRender PRIVATE "-Wno-return-type-c-linkage")
  # Nothing..
elseif (WIN32 OR ${ANDROID_TARGET_TAG} MATCHES "windows.*")
    list(APPEND host_common_SRC_FILES
    NativeSubWindow_win32.cpp)
  add_library (
    libOpenglRender
    SHARED
    ${host_common_SRC_FILES}
  )
  target_link_libraries (libOpenglRender PUBLIC -lgdi32)
elseif (UNIX OR ${ANDROID_TARGET_TAG} STREQUAL "linux-x86_64")
    list(APPEND host_common_SRC_FILES
    NativeSubWindow_x11.cpp
    )
  add_library (
    libOpenglRender
    SHARED
    ${host_common_SRC_FILES}
  )
  target_link_libraries (libOpenglRender PUBLIC -lX11 -lrt)
endif()

target_link_libraries (
  libOpenglRender
  PUBLIC
    libGLESv1_dec
    libGLESv2_dec
    librenderControl_dec
    libOpenglCodecCommon
    libOpenglRender_vulkan
)
target_link_libraries (
  libOpenglRender
  PRIVATE libemugl_common libOpenGLESDispatch libGLSnapshot
)

target_include_directories (libOpenglRender PRIVATE ../Translator/include)
set (
  standalone_common_SRC_FILES
  ${host_common_SRC_FILES}
  ../Translator/GLES_V2/ANGLEShaderParser.cpp
  standalone_common/angle-util/OSWindow.cpp
  standalone_common/SampleApplication.cpp
  standalone_common/SearchPathsSetup.cpp
  standalone_common/ShaderUtils.cpp
)

if(APPLE OR ${ANDROID_TARGET_TAG} STREQUAL "darwin-x86_64")
  add_library (
    libOpenglRender_standalone_common
    ${standalone_common_SRC_FILES}
  )
  target_link_libraries (
    libOpenglRender
    PUBLIC "-framework AppKit" "-Wl,-headerpad_max_install_names"
  )
  target_compile_options (
    libOpenglRender_standalone_common
    PRIVATE "-Wno-return-type-c-linkage"
  )
  # Nothing..
elseif (WIN32 OR ${ANDROID_TARGET_TAG} MATCHES "windows.*")
  add_library (
    libOpenglRender_standalone_common
    standalone_common/angle-util/windows/WindowsTimer.cpp
    standalone_common/angle-util/windows/win32/Win32Window.cpp
    ${standalone_common_SRC_FILES}
  )
  target_link_libraries (libOpenglRender PUBLIC "-lgdi32")
elseif (UNIX OR ${ANDROID_TARGET_TAG} STREQUAL "linux-x86_64")
  add_library (
    libOpenglRender_standalone_common
    standalone_common/angle-util/x11/X11Window.cpp
    ${standalone_common_SRC_FILES}
  )
  target_link_libraries (libOpenglRender PUBLIC -lX11 -lrt)
endif()

target_include_directories (
  libOpenglRender_standalone_common
  PUBLIC
    ${ANGLE_INCLUDE_DIRS}
    .
    standalone_common
    standalone_common/angle-util
    ../Translator/include
)

target_link_libraries (
  libOpenglRender_standalone_common
  PUBLIC
    -lm
    ${ANGLE_LIBRARIES}
    libOpenglRender
    libOpenglRender_vulkan
    libOpenGLESDispatch
    libGLSnapshot
    libemugl_common
    android-emu
    android-emu-base
)

add_executable (
  libOpenglRender_unittests
  samples/HelloTriangleImp.cpp
  tests/DefaultFramebufferBlit_unittest.cpp
  tests/FrameBuffer_unittest.cpp
  tests/GLSnapshot_unittest.cpp
  tests/GLSnapshotBuffers_unittest.cpp
  tests/GLSnapshotFramebufferControl_unittest.cpp
  tests/GLSnapshotFramebuffers_unittest.cpp
  tests/GLSnapshotMultisampling_unittest.cpp
  tests/GLSnapshotPixelOperations_unittest.cpp
  tests/GLSnapshotPixels_unittest.cpp
  tests/GLSnapshotPrograms_unittest.cpp
  tests/GLSnapshotRasterization_unittest.cpp
  tests/GLSnapshotRenderbuffers_unittest.cpp
  tests/GLSnapshotRendering_unittest.cpp
  tests/GLSnapshotShaders_unittest.cpp
  tests/GLSnapshotTestDispatch.cpp
  tests/GLSnapshotTesting.cpp
  tests/GLSnapshotTestStateUtils.cpp
  tests/GLSnapshotTextures_unittest.cpp
  tests/GLSnapshotTransformation_unittest.cpp
  tests/GLSnapshotVertexAttributes_unittest.cpp
  tests/GLTestUtils.cpp
  tests/OpenGL_unittest.cpp
  tests/OpenGLTestContext.cpp
  tests/StalePtrRegistry_unittest.cpp
  tests/TextureDraw_unittest.cpp
  tests/Vulkan_unittest.cpp
)

target_link_libraries (
  libOpenglRender_unittests
  PRIVATE libOpenglRender_standalone_common gmock_main
)
