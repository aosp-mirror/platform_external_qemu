prebuilt(ANGLE)
prebuilt(VULKAN)

# Adds the proper graphic driver dependencies to the given target.
function(add_opengl_dependencies TGT)

  # Copying of the shared lib is usually a nop for most code generators however some (like Xcode) like to place them in
  # their own spot, so we must make sure to copy them over.
  android_copy_shared_lib(${TGT} EGL_translator lib64EGL_translator)
  android_copy_shared_lib(${TGT} GLES_V2_translator lib64GLES_V2_translator)
  android_copy_shared_lib(${TGT} GLES_CM_translator lib64GLES_CM_translator)
  android_copy_shared_lib(${TGT} GLES12Translator lib64GLES12Translator)
  android_copy_shared_lib(${TGT} OpenglRender lib64OpenglRender)

  # And setup the depedencies
  android_target_dependency(${TGT} all "${SWIFTSHADER_DEPENDENCIES};${VULKAN_TEST_DEPENDENCIES}")
  android_target_properties(${TGT} linux-x86_64 "LINK_FLAGS>=-Wl,-rpath,'$ORIGIN/lib64/gles_swiftshader'")
  android_target_properties(${TGT} darwin-x86_64 "INSTALL_RPATH>=@loader_path/lib64/gles_swiftshader")
endfunction()

set(OSWindow_src standalone_common/angle-util/OSWindow.cpp)
set(OSWindow_darwin-x86_64_src standalone_common/angle-util/osx/OSXWindow.mm)
set(OSWindow_linux-x86_64_src standalone_common/angle-util/x11/X11Window.cpp)
set(OSWindow_windows_src standalone_common/angle-util/windows/WindowsTimer.cpp
    standalone_common/angle-util/windows/win32/Win32Window.cpp)
android_add_library(OSWindow)

android_target_link_libraries(OSWindow all PUBLIC android-emu-base)
android_target_link_libraries(OSWindow linux-x86_64 PUBLIC "-lX11" "-lrt")
android_target_link_libraries(OSWindow windows PUBLIC "-lgdi32")
android_target_link_libraries(OSWindow darwin-x86_64 PUBLIC "-framework AppKit")
android_target_link_libraries(OSWindow all PRIVATE emugl_base)
android_target_include_directories(OSWindow all PUBLIC ${ANDROID_EMUGL_DIR}/host/include standalone_common/angle-util)

set(OpenglRender_src
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
    YUVConverter.cpp)
set(OpenglRender_linux-x86_64_src NativeSubWindow_x11.cpp)
set(OpenglRender_darwin-x86_64_src NativeSubWindow_cocoa.m)
set(OpenglRender_windows_src NativeSubWindow_win32.cpp)
android_add_shared_library(OpenglRender)

# TODO: We are not consistent with our prefix naming, this should really be a global variable.
set_target_properties(OpenglRender PROPERTIES PREFIX "lib64")

android_target_link_libraries(OpenglRender
                              all
                              PUBLIC
                              GLESv1_dec
                              OpenglCodecCommon
                              emugl_common
                              gtest
                              GLESv2_dec
                              renderControl_dec
                              OpenglRender_vulkan
                              OpenglRender_vulkan_cereal
                              OpenGLESDispatch
                              GLSnapshot)
android_target_link_libraries(OpenglRender all PRIVATE emugl_base)
android_target_include_directories(OpenglRender all PUBLIC ${ANDROID_EMUGL_DIR}/host/libs/Translator/include .)
android_target_link_libraries(OpenglRender linux-x86_64 PRIVATE "-WL,--version-script=render_api.entries")
android_target_link_libraries(OpenglRender darwin-x86_64 PRIVATE "-WL,-exported_symbols_list,render_api.entries")

set(OpenglRender_standalone_common_src
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
    ../Translator/GLES_V2/ANGLEShaderParser.cpp
    standalone_common/SampleApplication.cpp
    standalone_common/SearchPathsSetup.cpp
    standalone_common/ShaderUtils.cpp)
set(OpenglRender_standalone_common_darwin-x86_64_src NativeSubWindow_cocoa.m)
set(OpenglRender_standalone_common_linux-x86_64_src NativeSubWindow_x11.cpp)
set(OpenglRender_standalone_common_windows_src NativeSubWindow_win32.cpp)
android_add_library(OpenglRender_standalone_common)

android_target_link_libraries(OpenglRender_standalone_common
                              all
                              PUBLIC
                              OpenglCodecCommon
                              emugl_common
                              OpenglRender
                              GLESv1_dec
                              GLESv2_dec
                              renderControl_dec
                              OpenglRender_vulkan
                              OpenglRender_vulkan_cereal
                              OSWindow)
android_target_link_libraries(OpenglRender_standalone_common all PRIVATE emugl_base)

android_target_include_directories(OpenglRender_standalone_common all PRIVATE ${ANGLE_INCLUDE_DIRS})
android_target_include_directories(OpenglRender_standalone_common
                                   all
                                   PUBLIC
                                   ${ANDROID_EMUGL_DIR}/host/include/OpenglRender
                                   standalone_common
                                   vulkan
                                   ${ANDROID_EMUGL_DIR}/host/include/vulkan)

# Unit tests
set(OpenglRender_unittests_src
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
    tests/Vulkan_unittest.cpp)
android_add_test(OpenglRender_unittests)
android_target_link_libraries(OpenglRender_unittests
                              all
                              PRIVATE
                              android-emu-base
                              gmock_main
                              OpenglRender_standalone_common)

add_opengl_dependencies(OpenglRender_unittests)

set(HelloTriangle_src samples/HelloTriangle.cpp samples/HelloTriangleImp.cpp)
android_add_executable(HelloTriangle)
android_target_link_libraries(HelloTriangle
                              all
                              PUBLIC
                              OpenglRender_standalone_common
                              OpenglCodecCommon
                              emugl_common
                              gmock_main
                              OpenglRender
                              GLESv1_dec
                              GLESv2_dec
                              renderControl_dec
                              OpenglRender_vulkan
                              OpenglRender_vulkan_cereal
                              OSWindow)
add_opengl_dependencies(HelloTriangle)

set(HelloSurfaceFlinger_src samples/HelloSurfaceFlinger.cpp)
android_add_executable(HelloSurfaceFlinger)
android_target_link_libraries(HelloSurfaceFlinger
                              all
                              PUBLIC
                              OpenglRender_standalone_common
                              OpenglCodecCommon
                              emugl_common
                              gmock_main
                              OpenglRender
                              GLESv1_dec
                              GLESv2_dec
                              renderControl_dec
                              OpenglRender_vulkan
                              OpenglRender_vulkan_cereal
                              OSWindow)
add_opengl_dependencies(HelloSurfaceFlinger)

set(CreateDestroyContext_src samples/CreateDestroyContext.cpp)
android_add_executable(CreateDestroyContext)
android_target_link_libraries(CreateDestroyContext
                              all
                              PUBLIC
                              OpenglRender_standalone_common
                              OpenglCodecCommon
                              emugl_common
                              gmock_main
                              OpenglRender
                              GLESv1_dec
                              GLESv2_dec
                              renderControl_dec
                              OpenglRender_vulkan
                              OpenglRender_vulkan_cereal
                              OSWindow)
add_opengl_dependencies(CreateDestroyContext)

set(HelloHostComposition_src samples/HelloHostComposition.cpp)
android_add_executable(HelloHostComposition)
android_target_link_libraries(HelloHostComposition
                              all
                              PUBLIC
                              OpenglRender_standalone_common
                              OpenglCodecCommon
                              emugl_common
                              gmock_main
                              OpenglRender
                              GLESv1_dec
                              GLESv2_dec
                              renderControl_dec
                              OpenglRender_vulkan
                              OpenglRender_vulkan_cereal
                              OSWindow)
add_opengl_dependencies(HelloHostComposition)
