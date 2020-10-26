prebuilt(ANGLE)
prebuilt(VULKAN)

# Adds the proper graphic driver dependencies to the given target.
function(add_opengl_dependencies TGT)
  # And setup the depedencies
  android_target_dependency(${TGT} all SWIFTSHADER_DEPENDENCIES)
  android_target_dependency(${TGT} all ANGLE_DEPENDENCIES)
  android_target_dependency(${TGT} all VULKAN_DEPENDENCIES)
  android_target_dependency(${TGT} all VULKAN_TEST_DEPENDENCIES)
  android_target_properties(
    ${TGT} linux-x86_64
    "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/gles_swiftshader'")
  android_target_properties(
    ${TGT} linux-aarch64
    "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/gles_swiftshader'")
  android_target_properties(
    ${TGT} darwin-x86_64 "INSTALL_RPATH>=@loader_path/lib64/gles_swiftshader")
endfunction()

android_add_library(
  TARGET OSWindow
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      standalone_common/angle-util/OSWindow.cpp
  DARWIN standalone_common/angle-util/osx/OSXWindow.mm
  LINUX standalone_common/angle-util/x11/X11Window.cpp
  WINDOWS standalone_common/angle-util/windows/win32/Win32Window.cpp
          standalone_common/angle-util/windows/WindowsTimer.cpp)

android_target_link_libraries(OSWindow linux-x86_64 PUBLIC "-lX11" "-lrt"
                                                           android-emu-shared)
android_target_link_libraries(OSWindow linux-aarch64 PUBLIC "-lX11" "-lrt"
                                                            android-emu-shared)

android_target_link_libraries(OSWindow windows PUBLIC "gdi32::gdi32")
android_target_link_libraries(OSWindow darwin-x86_64 PUBLIC "-framework AppKit")
target_link_libraries(OSWindow PRIVATE emugl_base)
target_include_directories(OSWindow PUBLIC ${ANDROID_EMUGL_DIR}/host/include
                                           standalone_common/angle-util)

android_add_library(
  TARGET OpenglRender_vulkan
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      vulkan/VkAndroidNativeBuffer.cpp
      vulkan/VkCommonOperations.cpp
      vulkan/VkDecoder.cpp
      vulkan/VkDecoderGlobalState.cpp
      vulkan/VkDecoderSnapshot.cpp
      vulkan/VkReconstruction.cpp
      vulkan/VulkanDispatch.cpp
      vulkan/VulkanHandleMapping.cpp
      vulkan/VulkanStream.cpp)

target_link_libraries(
  OpenglRender_vulkan PUBLIC emugl_common OpenglRender_vulkan_cereal
  PRIVATE emugl_base GLcommon)
android_target_compile_definitions(OpenglRender_vulkan windows PRIVATE
                                   -DVK_USE_PLATFORM_WIN32_KHR)
target_compile_options(
  OpenglRender_vulkan PRIVATE -fvisibility=hidden -Wno-unused-value
                              -Wno-return-type -Wno-return-type-c-linkage)
target_include_directories(
  OpenglRender_vulkan
  PUBLIC ./vulkan/cereal/ ./vulkan/
  PRIVATE .
          ${ANDROID_EMUGL_DIR}/../android-emu
          ${ANDROID_EMUGL_DIR}/shared/
          ${ANDROID_EMUGL_DIR}/shared/OpenglCodecCommon
          ${ANDROID_EMUGL_DIR}/host/include
          ${ANDROID_EMUGL_DIR}/host/include/OpenglRender)

android_add_library(
  TARGET OpenglRender
  SHARED
  LICENSE Apache-2.0
  SRC # cmake-format: sortable
      ChannelStream.cpp
      ColorBuffer.cpp
      FbConfig.cpp
      FenceSync.cpp
      FrameBuffer.cpp
      GLESVersionDetector.cpp
      PostWorker.cpp
      ReadbackWorker.cpp
      ReadBuffer.cpp
      render_api.cpp
      RenderChannelImpl.cpp
      RenderContext.cpp
      RenderControl.cpp
      RendererImpl.cpp
      RenderLibImpl.cpp
      RenderThread.cpp
      RenderThreadInfo.cpp
      RenderWindow.cpp
      RingStream.cpp
      SyncThread.cpp
      TextureDraw.cpp
      TextureResize.cpp
      WindowSurface.cpp
      YUVConverter.cpp
  LINUX NativeSubWindow_x11.cpp
  DARWIN NativeSubWindow_cocoa.m
  WINDOWS NativeSubWindow_win32.cpp)

target_link_libraries(
  OpenglRender
  PUBLIC GLESv1_dec
         OpenglCodecCommon
         emugl_common
         GLESv2_dec
         renderControl_dec
         OpenglRender_vulkan
         OpenGLESDispatch
         GLSnapshot)
target_compile_definitions(OpenglRender PRIVATE -DVK_ANDROID_native_buffer)
android_target_compile_definitions(OpenglRender windows PRIVATE
                                   -DVK_USE_PLATFORM_WIN32_KHR)
target_compile_options(OpenglRender PRIVATE -fvisibility=hidden)
target_compile_options(OpenglRender PUBLIC -Wno-return-type-c-linkage)
target_link_libraries(OpenglRender PRIVATE emugl_base)
target_include_directories(
  OpenglRender PUBLIC ${ANDROID_EMUGL_DIR}/host/libs/Translator/include .
                      ${ANDROID_EMUGL_DIR}/host/include/OpenglRender)
android_target_link_libraries(OpenglRender linux-x86_64
                              PRIVATE "-WL,--version-script=render_api.entries")

android_target_link_libraries(
  OpenglRender darwin-x86_64
  PRIVATE "-WL,-exported_symbols_list,render_api.entries")
android_install_shared(OpenglRender)

if(NOT LINUX_AARCH64)

  android_add_library(
    TARGET OpenglRender_standalone_common
    LICENSE Apache-2.0
    SRC # cmake-format: sortable
        ../Translator/GLES_V2/ANGLEShaderParser.cpp
        ChannelStream.cpp
        ColorBuffer.cpp
        FbConfig.cpp
        FenceSync.cpp
        FrameBuffer.cpp
        GLESVersionDetector.cpp
        PostWorker.cpp
        ReadbackWorker.cpp
        ReadBuffer.cpp
        render_api.cpp
        RenderChannelImpl.cpp
        RenderContext.cpp
        RenderControl.cpp
        RendererImpl.cpp
        RenderLibImpl.cpp
        RenderThread.cpp
        RenderThreadInfo.cpp
        RenderWindow.cpp
        RingStream.cpp
        standalone_common/SampleApplication.cpp
        standalone_common/SearchPathsSetup.cpp
        standalone_common/ShaderUtils.cpp
        SyncThread.cpp
        TextureDraw.cpp
        TextureResize.cpp
        WindowSurface.cpp
        YUVConverter.cpp
    LINUX NativeSubWindow_x11.cpp
    DARWIN NativeSubWindow_cocoa.m
    WINDOWS NativeSubWindow_win32.cpp)

  target_link_libraries(
    OpenglRender_standalone_common
    PUBLIC OpenglCodecCommon
           emugl_common
           OpenglRender
           GLESv1_dec
           GLESv2_dec
           renderControl_dec
           OpenglRender_vulkan
           OSWindow)
  target_link_libraries(OpenglRender_standalone_common PRIVATE emugl_base
                                                               ANGLE::ANGLE)
  target_compile_options(OpenglRender_standalone_common
                         PRIVATE -fvisibility=hidden)
  android_target_compile_definitions(OpenglRender_standalone_common windows
                                     PRIVATE -DVK_USE_PLATFORM_WIN32_KHR)
  target_include_directories(
    OpenglRender_standalone_common
    PUBLIC ${ANDROID_EMUGL_DIR}/host/include/OpenglRender standalone_common
           vulkan ${ANDROID_EMUGL_DIR}/host/include/vulkan)

  # Note! We build the test but are not registering it, as the darwin build has
  # some concurrency issues with these tests.
  android_add_test(
    TARGET OpenglRender_unittests
    SRC # cmake-format: sortable
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
        tests/TextureDraw_unittest.cpp)
  target_link_libraries(
    OpenglRender_unittests PRIVATE OpenglRender_standalone_common
                                   android-emu-test-launcher android-emu)
  android_target_compile_definitions(OpenglRender_unittests windows PRIVATE
                                     -DVK_USE_PLATFORM_WIN32_KHR)
  add_opengl_dependencies(OpenglRender_unittests)

  # Unit tests
  if(NOT WINDOWS)
    android_add_test(
      TARGET OpenglRender_vulkan_unittests
      SRC # cmake-format: sortable
          tests/Vulkan_unittest.cpp vulkan/VulkanStream_unittest.cpp)
    add_opengl_dependencies(OpenglRender_vulkan_unittests)
    target_link_libraries(OpenglRender_vulkan_unittests
                          PRIVATE android-emu-test-launcher android-emu)
    target_link_libraries(
      OpenglRender_vulkan_unittests PUBLIC OpenglRender_standalone_common
                                           OpenglRender_vulkan gmock_main)
    target_include_directories(
      OpenglRender_vulkan_unittests
      PRIVATE . cereal ${ANDROID_EMUGL_DIR}/host/include/OpenglRender
              ${ANDROID_EMUGL_DIR}/shared/OpenglCodecCommon
              ${ANDROID_EMUGL_DIR}/host/include/vulkan)
    android_target_compile_definitions(OpenglRender_vulkan_unittests windows
                                       PRIVATE -DVK_USE_PLATFORM_WIN32_KHR)
  endif()

  android_add_executable(
    TARGET HelloTriangle NODISTRIBUTE
    SRC # cmake-format: sortable
        samples/HelloTriangle.cpp samples/HelloTriangleImp.cpp)
  target_link_libraries(
    HelloTriangle
    PUBLIC OpenglRender_standalone_common
           OpenglCodecCommon
           android-emu-base
           emugl_common
           gmock_main
           OpenglRender
           GLESv1_dec
           GLESv2_dec
           renderControl_dec
           OpenglRender_vulkan
           OSWindow)
  add_opengl_dependencies(HelloTriangle)

  android_add_executable(
    TARGET HelloSurfaceFlinger NODISTRIBUTE SRC # cmake-format: sortable
                                                samples/HelloSurfaceFlinger.cpp)
  target_link_libraries(
    HelloSurfaceFlinger
    PUBLIC OpenglRender_standalone_common
           OpenglCodecCommon
           android-emu-base
           emugl_common
           gmock_main
           OpenglRender
           GLESv1_dec
           GLESv2_dec
           renderControl_dec
           OpenglRender_vulkan
           OSWindow)
  add_opengl_dependencies(HelloSurfaceFlinger)

  android_add_executable(
    TARGET CreateDestroyContext LICENSE Apache-2.0
    SRC # cmake-format: sortable
        samples/CreateDestroyContext.cpp)
  target_link_libraries(
    CreateDestroyContext
    PUBLIC OpenglRender_standalone_common
           OpenglCodecCommon
           android-emu-base
           emugl_common
           gmock_main
           OpenglRender
           GLESv1_dec
           GLESv2_dec
           renderControl_dec
           OpenglRender_vulkan
           OSWindow)
  add_opengl_dependencies(CreateDestroyContext)

  android_add_executable(
    TARGET HelloHostComposition NODISTRIBUTE
    SRC # cmake-format: sortable
        samples/HelloHostComposition.cpp)
  target_link_libraries(
    HelloHostComposition
    PUBLIC OpenglRender_standalone_common
           OpenglCodecCommon
           android-emu-base
           emugl_common
           gmock_main
           OpenglRender
           GLESv1_dec
           GLESv2_dec
           renderControl_dec
           OpenglRender_vulkan
           OSWindow)
  add_opengl_dependencies(HelloHostComposition)

  android_add_executable(
    TARGET HelloVulkan NODISTRIBUTE SRC # cmake-format: sortable
                                        samples/HelloVulkan.cpp)
  target_link_libraries(
    HelloVulkan
    PUBLIC OpenglRender_standalone_common
           OpenglCodecCommon
           android-emu-base
           emugl_common
           gmock_main
           OpenglRender
           GLESv1_dec
           GLESv2_dec
           renderControl_dec
           OpenglRender_vulkan
           OSWindow)
  add_opengl_dependencies(HelloVulkan)

endif()
