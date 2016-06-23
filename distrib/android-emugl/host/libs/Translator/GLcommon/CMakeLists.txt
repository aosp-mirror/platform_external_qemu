
add_library(GLcommon STATIC
    GLDispatch.cpp
    GLutils.cpp
    GLEScontext.cpp
    GLESvalidate.cpp
    GLESpointer.cpp
    GLESbuffer.cpp
    RangeManip.cpp
    TextureUtils.cpp
    PaletteTexture.cpp
    etc1.cpp
    objectNameManager.cpp
    FramebufferData.cpp
    )

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(GLCOMMON_PLATFORM_LIBS INTERFACE -ldl -Wl,-Bsymbolic)
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(GLCOMMON_PLATFORM_LIBS INTERFACE INTERFACE -lgdi32 -Wl,--add-stdcall-alias)
endif()

target_link_libraries(GLcommon
    ${GLCOMMON_PLATFORM_LIBS}
    emugl_common
    )

target_include_directories(GLcommon
    PUBLIC
    ../include         # android-emugl/host/libs/Translator/include
    # not sure why these next two dirs are necessary,
    # these path are a public includes from emugl_common
    ${REPO_QEMU}
    ${REPO_QEMU}/distrib/android-emugl/shared
    )
#[[

### EGL host implementation ########################

$(call emugl-begin-static-library,libGLcommon)

LOCAL_SRC_FILES := $(host_common_SRC_FILES)
$(call emugl-export,C_INCLUDES,$(LOCAL_PATH)/../include $(EMUGL_PATH)/shared)
$(call emugl-export,STATIC_LIBRARIES, libemugl_common)

$(call emugl-end-module)

]]