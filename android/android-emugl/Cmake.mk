$(call start-cmake-project,emugl)

PRODUCED_STATIC_LIBRARIES:=HOST_libemugl_gtest_host \
	lib64OSWindow \
	lib64OpenglRender_standalone_common \
	lib64OpenglRender_vulkan \
	lib64OpenglRender_vulkan_cereal \
	libGLESv1_dec \
	libGLESv2_dec \
	libGLSnapshot \
	libGLcommon \
	libOpenGLESDispatch \
	libOpenglCodecCommon \
	lib_renderControl_dec \
	libemugl_common \
	libemugl_gtest \

PRODUCED_SHARED_LIBRARIES:=gralloc.goldfish  \
	gralloc.ranchu  \
	lib64EGL_translator  \
	lib64GLES12Translator  \
	lib64GLES_CM_translator  \
	lib64GLES_V2_translator  \
	lib64OpenglRender  \
	lib64emugl_test_shared_library  \
	libEGL_emulation  \
	libGLESv1_CM_emulation  \
	libGLESv1_enc  \
	libGLESv2_emulation  \
	libGLESv2_enc  \
	libOpenglCodecCommon_host  \
	libOpenglSystemCommon  \
	lib_renderControl_enc  \
	libaemugraphics  \
	libcutils  \
	libgui  \
	liblog  \
	$(call end-cmake-project)

$(call start-cmake-project,foo)
#PRODUCED_EXECUTABLES:=CreateDestroyContext  \
	HelloHostComposition  \
	HelloSurfaceFlinger  \
	HelloTriangle  \


#PRODUCED_TESTS:= emugl64_common_host_unittests  \
	emugl_combined_unittests  \
	lib64GLcommon_unittests  \
	lib64OpenglRender_unittests  \
	lib64OpenglRender_vulkan_unittests  \
	libaemugraphics_toplevel_unittests  \
	libgui_unittests  \

$(call end-cmake-project)

$
