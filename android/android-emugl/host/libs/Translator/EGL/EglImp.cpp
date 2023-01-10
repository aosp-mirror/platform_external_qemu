/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifdef _WIN32
#undef EGLAPI
#define EGLAPI
#define EGLAPIENTRY
#endif

#include <GLcommon/GLESmacros.h>
#include "GLcommon/GLEScontext.h"
#include "GLcommon/GLutils.h"
#include "GLcommon/TextureData.h"
#include "GLcommon/TranslatorIfaces.h"
#include "ThreadInfo.h"
#include "android/base/files/Stream.h"
#include "android/base/system/System.h"
#include "android/utils/GfxstreamFatalError.h"
#include "emugl/common/logging.h"
#include "emugl/common/shared_library.h"

#include "EglWindowSurface.h"
#include "EglPbufferSurface.h"
#include "EglGlobalInfo.h"
#include "EglThreadInfo.h"
#include "EglValidate.h"
#include "EglDisplay.h"
#include "EglContext.h"
#include "EglConfig.h"
#include "EglOsApi.h"
#include "ClientAPIExts.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAJOR          1
#define MINOR          4

//declarations

namespace translator {
namespace egl {

ImagePtr getEGLImage(unsigned int imageId);
GLEScontext* getGLESContext();
GlLibrary* getGlLibrary();
static bool createAndBindAuxiliaryContext(
    EGLContext* context_out, EGLSurface* surface_out);
static bool unbindAndDestroyAuxiliaryContext(
    EGLContext context, EGLSurface surface);
static bool bindAuxiliaryContext(
    EGLContext context, EGLSurface surface);
static bool unbindAuxiliaryContext();

} // namespace translator
} // namespace egl

#define tls_thread  EglThreadInfo::get()

EglGlobalInfo* g_eglInfo = NULL;
emugl::Mutex  s_eglLock;
emugl::Mutex  s_surfaceDestroyLock;

void initGlobalInfo()
{
    emugl::Mutex::AutoLock mutex(s_eglLock);
    if (!g_eglInfo) {
        g_eglInfo = EglGlobalInfo::getInstance();
    }
}

static const EGLiface s_eglIface = {
    .getGLESContext = translator::egl::getGLESContext,
    .getEGLImage = translator::egl::getEGLImage,
    .eglGetGlLibrary = translator::egl::getGlLibrary,
    .createAndBindAuxiliaryContext = translator::egl::createAndBindAuxiliaryContext,
    .unbindAndDestroyAuxiliaryContext = translator::egl::unbindAndDestroyAuxiliaryContext,
    .bindAuxiliaryContext = translator::egl::bindAuxiliaryContext,
    .unbindAuxiliaryContext = translator::egl::unbindAuxiliaryContext,
};

static void initGLESx(GLESVersion version) {
    const GLESiface* iface = g_eglInfo->getIface(version);
    if (!iface) {
        ERR("EGL failed to initialize GLESv%d; incompatible interface", version);
        return;
    }
    iface->initGLESx(EglGlobalInfo::isEgl2Egl());
}

/*****************************************  supported extensions  ***********************************************************************/

namespace translator {
namespace egl {

EGLAPI EGLImageKHR EGLAPIENTRY eglCreateImageKHR(EGLDisplay display, EGLContext context, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list);
EGLAPI EGLBoolean EGLAPIENTRY eglDestroyImageKHR(EGLDisplay display, EGLImageKHR image);
EGLAPI EGLSyncKHR EGLAPIENTRY eglCreateSyncKHR(EGLDisplay display, EGLenum type, const EGLint* attribs);
EGLAPI EGLint EGLAPIENTRY eglClientWaitSyncKHR(EGLDisplay display, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout);
EGLAPI EGLBoolean EGLAPIENTRY eglDestroySyncKHR(EGLDisplay display, EGLSyncKHR sync);
EGLAPI EGLint EGLAPIENTRY eglGetMaxGLESVersion(EGLDisplay display);
EGLAPI EGLint EGLAPIENTRY eglWaitSyncKHR(EGLDisplay display, EGLSyncKHR sync, EGLint flags);
EGLAPI void EGLAPIENTRY eglBlitFromCurrentReadBufferANDROID(EGLDisplay display, EGLImageKHR image);
EGLAPI void* EGLAPIENTRY eglSetImageFenceANDROID(EGLDisplay display, EGLImageKHR image);
EGLAPI void EGLAPIENTRY eglWaitImageFenceANDROID(EGLDisplay display, void* fence);
EGLAPI void EGLAPIENTRY eglAddLibrarySearchPathANDROID(const char* path);
EGLAPI EGLBoolean EGLAPIENTRY eglQueryVulkanInteropSupportANDROID(void);
EGLAPI EGLBoolean EGLAPIENTRY eglGetSyncAttribKHR(EGLDisplay display, EGLSyncKHR sync, EGLint attribute, EGLint *value);

EGLAPI EGLBoolean EGLAPIENTRY eglSaveConfig(EGLDisplay display, EGLConfig config, EGLStream stream);
EGLAPI EGLConfig EGLAPIENTRY eglLoadConfig(EGLDisplay display, EGLStream stream);

EGLAPI EGLBoolean EGLAPIENTRY eglPreSaveContext(EGLDisplay display, EGLContext contex, EGLStream stream);
EGLAPI EGLBoolean EGLAPIENTRY eglSaveContext(EGLDisplay display, EGLContext contex, EGLStream stream);
EGLAPI EGLBoolean EGLAPIENTRY eglPostSaveContext(EGLDisplay display, EGLContext context, EGLStream stream);
EGLAPI EGLContext EGLAPIENTRY eglLoadContext(EGLDisplay display, const EGLint *attrib_list, EGLStream stream);

EGLAPI EGLBoolean EGLAPIENTRY eglSaveAllImages(EGLDisplay display,
                                               EGLStream stream,
                                               const void* textureSaver);
EGLAPI EGLBoolean EGLAPIENTRY eglLoadAllImages(EGLDisplay display,
                                               EGLStream stream,
                                               const void* textureLoader);
EGLAPI EGLBoolean EGLAPIENTRY eglPostLoadAllImages(EGLDisplay display, EGLStream stream);
EGLAPI void EGLAPIENTRY eglUseOsEglApi(EGLBoolean enable);
EGLAPI void EGLAPIENTRY eglSetMaxGLESVersion(EGLint version);
EGLAPI void EGLAPIENTRY eglFillUsages(void* usages);

EGLAPI EGLDisplay EGLAPIENTRY eglGetNativeDisplayANDROID(EGLDisplay);
EGLAPI EGLContext EGLAPIENTRY eglGetNativeContextANDROID(EGLDisplay, EGLContext);
EGLAPI EGLImage EGLAPIENTRY eglGetNativeImageANDROID(EGLDisplay, EGLImage);
EGLAPI EGLBoolean EGLAPIENTRY eglSetImageInfoANDROID(EGLDisplay, EGLImage, EGLint, EGLint, EGLint);
EGLAPI EGLImage EGLAPIENTRY eglImportImageANDROID(EGLDisplay, EGLImage);

} // namespace translator
} // namespace egl

namespace translator {
namespace egl {

static const ExtensionDescriptor s_eglExtensions[] = {
        {"eglCreateImageKHR" ,
                (__eglMustCastToProperFunctionPointerType)eglCreateImageKHR },
        {"eglDestroyImageKHR",
                (__eglMustCastToProperFunctionPointerType)eglDestroyImageKHR },
        {"eglCreateSyncKHR" ,
                (__eglMustCastToProperFunctionPointerType)eglCreateSyncKHR },
        {"eglClientWaitSyncKHR",
                (__eglMustCastToProperFunctionPointerType)eglClientWaitSyncKHR },
        {"eglDestroySyncKHR",
                (__eglMustCastToProperFunctionPointerType)eglDestroySyncKHR },
        {"eglGetMaxGLESVersion",
                (__eglMustCastToProperFunctionPointerType)eglGetMaxGLESVersion },
        {"eglWaitSyncKHR",
                (__eglMustCastToProperFunctionPointerType)eglWaitSyncKHR },
        {"eglBlitFromCurrentReadBufferANDROID",
                (__eglMustCastToProperFunctionPointerType)eglBlitFromCurrentReadBufferANDROID },
        {"eglSetImageFenceANDROID",
                (__eglMustCastToProperFunctionPointerType)eglSetImageFenceANDROID },
        {"eglWaitImageFenceANDROID",
                (__eglMustCastToProperFunctionPointerType)eglWaitImageFenceANDROID },
        {"eglAddLibrarySearchPathANDROID",
                (__eglMustCastToProperFunctionPointerType)eglAddLibrarySearchPathANDROID },
        {"eglQueryVulkanInteropSupportANDROID",
                (__eglMustCastToProperFunctionPointerType)eglQueryVulkanInteropSupportANDROID },
        {"eglGetSyncAttribKHR",
                (__eglMustCastToProperFunctionPointerType)eglGetSyncAttribKHR },
};

static const int s_eglExtensionsSize =
        sizeof(s_eglExtensions) / sizeof(ExtensionDescriptor);

} // namespace translator
} // namespace egl

/****************************************************************************************************************************************/
//macros for accessing global egl info & tls objects

extern "C" {
}

namespace translator {
namespace egl {

#define CURRENT_THREAD() do {} while (0);

#define RETURN_ERROR(ret,err)                                \
        CURRENT_THREAD()                                     \
        if(tls_thread->getError() == EGL_SUCCESS) {          \
          tls_thread->setError(err);                         \
        }                                                    \
        return ret;

#define VALIDATE_DISPLAY_RETURN(EGLDisplay, ret)                \
    MEM_TRACE_IF(strncmp(__FUNCTION__, "egl", 3) == 0, "EMUGL") \
    EglDisplay* dpy = g_eglInfo->getDisplay(EGLDisplay);        \
    if (!dpy) {                                                 \
        RETURN_ERROR(ret, EGL_BAD_DISPLAY);                     \
    }                                                           \
    if (!dpy->isInitialize()) {                                 \
        RETURN_ERROR(ret, EGL_NOT_INITIALIZED);                 \
    }

#define VALIDATE_CONFIG_RETURN(EGLConfig,ret)                \
        EglConfig* cfg = dpy->getConfig(EGLConfig);          \
        if(!cfg) {                                           \
            RETURN_ERROR(ret,EGL_BAD_CONFIG);                \
        }

#define VALIDATE_SURFACE_RETURN(EGLSurface,ret,varName)      \
        SurfacePtr varName = dpy->getSurface(EGLSurface);    \
        if(!varName.get()) {                                 \
            RETURN_ERROR(ret,EGL_BAD_SURFACE);               \
        }

#define VALIDATE_CONTEXT_RETURN(EGLContext,ret)              \
        ContextPtr ctx = dpy->getContext(EGLContext);        \
        if(!ctx.get()) {                                     \
            RETURN_ERROR(ret,EGL_BAD_CONTEXT);               \
        }


#define VALIDATE_DISPLAY(EGLDisplay) \
        VALIDATE_DISPLAY_RETURN(EGLDisplay,EGL_FALSE)

#define VALIDATE_CONFIG(EGLConfig)   \
        VALIDATE_CONFIG_RETURN(EGLConfig,EGL_FALSE)

#define VALIDATE_SURFACE(EGLSurface,varName) \
        VALIDATE_SURFACE_RETURN(EGLSurface,EGL_FALSE,varName)

#define VALIDATE_CONTEXT(EGLContext) \
        VALIDATE_CONTEXT_RETURN(EGLContext,EGL_FALSE)


GLEScontext* getGLESContext()
{
    ThreadInfo* thread  = getThreadInfo();
    return thread->glesContext;
}

GlLibrary* getGlLibrary() {
    return EglGlobalInfo::getInstance()->getOsEngine()->getGlLibrary();
}

EGLAPI EGLDisplay EGLAPIENTRY eglGetDisplay(EGLNativeDisplayType display_id) {
    MEM_TRACE("EMUGL");
    EglDisplay* dpy = NULL;
    EglOS::Display* internalDisplay = NULL;

    initGlobalInfo();

    if ((dpy = g_eglInfo->getDisplay(display_id))) {
        return dpy;
    }
    if (display_id != EGL_DEFAULT_DISPLAY) {
        return EGL_NO_DISPLAY;
    }
    internalDisplay = g_eglInfo->getDefaultNativeDisplay();
    dpy = g_eglInfo->addDisplay(display_id,internalDisplay);
    if(!dpy) {
        return EGL_NO_DISPLAY;
    }
    return dpy;
}

} // namespace translator
} // namespace egl

#define TRANSLATOR_GETIFACE_NAME "__translator_getIfaces"

extern "C" {
GLESiface* static_translator_glescm_getIfaces(const EGLiface*);
GL_APICALL GLESiface* GL_APIENTRY static_translator_glesv2_getIfaces(const EGLiface*);
}; // extern "C"

#define STATIC_TRANSLATOR_GETIFACE_NAME_GLES_CM static_translator_glescm_getIfaces
#define STATIC_TRANSLATOR_GETIFACE_NAME_GLES_V2 static_translator_glesv2_getIfaces

#define LIB_GLES_CM_NAME EMUGL_LIBNAME("GLES_CM_translator")
#define LIB_GLES_V2_NAME EMUGL_LIBNAME("GLES_V2_translator")

static __translator_getGLESIfaceFunc loadIfaces(const char* libName,
                                                char* error,
                                                size_t errorSize) {

    if (!strcmp(libName, LIB_GLES_CM_NAME)) {
        return STATIC_TRANSLATOR_GETIFACE_NAME_GLES_CM;
    }

    if (!strcmp(libName, LIB_GLES_V2_NAME)) {
        return STATIC_TRANSLATOR_GETIFACE_NAME_GLES_V2;
    }

    return 0;
}

namespace translator {
namespace egl {

EGLAPI EGLBoolean EGLAPIENTRY eglInitialize(EGLDisplay display, EGLint *major, EGLint *minor) {
    MEM_TRACE("EMUGL");

    initGlobalInfo();

    EglDisplay* dpy = g_eglInfo->getDisplay(display);
    if(!dpy) {
         RETURN_ERROR(EGL_FALSE,EGL_BAD_DISPLAY);
    }

    if(major) *major = MAJOR;
    if(minor) *minor = MINOR;

    __translator_getGLESIfaceFunc func  = NULL;
    int renderableType = EGL_OPENGL_ES_BIT;

    g_eglInfo->setEglIface(&s_eglIface);

    char error[256];
    // When running on top of another GLES library, our GLES1
    // translator uses the GLES library's underlying GLES3.
    if(!g_eglInfo->getIface(GLES_1_1)) {
        func  = loadIfaces(LIB_GLES_CM_NAME, error, sizeof(error));
        if (func) {
            g_eglInfo->setIface(func(&s_eglIface),GLES_1_1);
        } else {
           fprintf(stderr, "%s: Could not find ifaces for GLES CM 1.1 [%s]\n",
                   __FUNCTION__, error);
           return EGL_FALSE;
        }
        initGLESx(GLES_1_1);
    }
    if(!g_eglInfo->getIface(GLES_2_0)) {
        func  = loadIfaces(LIB_GLES_V2_NAME, error, sizeof(error));
        if (func) {
            renderableType |= EGL_OPENGL_ES2_BIT;
            g_eglInfo->setIface(func(&s_eglIface),GLES_2_0);
        } else {
           fprintf(stderr, "%s: Could not find ifaces for GLES 2.0 [%s]\n",
                   __FUNCTION__, error);
        }
        initGLESx(GLES_2_0);
    }
    if(!g_eglInfo->getIface(GLES_3_0)) {
        func  = loadIfaces(LIB_GLES_V2_NAME, error, sizeof(error));
        if (func) {
            renderableType |= EGL_OPENGL_ES2_BIT | EGL_OPENGL_ES3_BIT;
            g_eglInfo->setIface(func(&s_eglIface),GLES_3_0);
        } else {
           fprintf(stderr, "%s: Could not find ifaces for GLES 3.x [%s]\n",
                   __FUNCTION__, error);
        }
        initGLESx(GLES_3_0);
    }
    if(!g_eglInfo->getIface(GLES_3_1)) {
        func  = loadIfaces(LIB_GLES_V2_NAME, error, sizeof(error));
        if (func) {
            renderableType |= EGL_OPENGL_ES2_BIT | EGL_OPENGL_ES3_BIT;
            g_eglInfo->setIface(func(&s_eglIface),GLES_3_1);
        } else {
           fprintf(stderr, "%s: Could not find ifaces for GLES 3.x [%s]\n",
                   __FUNCTION__, error);
        }
        initGLESx(GLES_3_1);
    }
    dpy->initialize(renderableType);
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglTerminate(EGLDisplay display) {
    VALIDATE_DISPLAY(display);
    dpy->terminate();
    return EGL_TRUE;
}

EGLAPI const char * EGLAPIENTRY eglQueryString(EGLDisplay display, EGLint name) {
    VALIDATE_DISPLAY(display);
    static const char* vendor     = "Google";
    static const char* version    = "1.4";
    static const char* extensions = "EGL_KHR_image EGL_KHR_image_base "
                                    "EGL_KHR_gl_texture_2D_image "
                                    "EGL_ANDROID_recordable ";
    if(!EglValidate::stringName(name)) {
        RETURN_ERROR(NULL,EGL_BAD_PARAMETER);
    }
    switch(name) {
    case EGL_VENDOR:
        return vendor;
    case EGL_VERSION:
        return version;
    case EGL_EXTENSIONS:
        return extensions;
    }
    return NULL;
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigs(EGLDisplay display, EGLConfig *configs,
             EGLint config_size, EGLint *num_config) {
    VALIDATE_DISPLAY(display);
    if(!num_config) {
        RETURN_ERROR(EGL_FALSE,EGL_BAD_PARAMETER);
    }

    if(configs == NULL) {
        *num_config = dpy->nConfigs();
    } else {
        *num_config = dpy->getConfigs(configs,config_size);
    }

    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglChooseConfig(EGLDisplay display, const EGLint *attrib_list,
               EGLConfig *configs, EGLint config_size,
               EGLint *num_config) {
    CHOOSE_CONFIG_DLOG("eglChooseConfig: begin. validating arguments...");

    VALIDATE_DISPLAY(display);
    if(!num_config) {
         CHOOSE_CONFIG_DLOG("num_config is NULL. issue EGL_BAD_PARAMETER");
         RETURN_ERROR(EGL_FALSE,EGL_BAD_PARAMETER);
    }

        //selection defaults
        // NOTE: Some variables below are commented out to reduce compiler warnings.
        // TODO(digit): Look if these variables are really needed or not, and if so
        // fix the code to do it properly.
        EGLint      surface_type       = EGL_WINDOW_BIT;
        EGLint      renderable_type    = EGL_OPENGL_ES_BIT;
        //EGLBoolean  bind_to_tex_rgb    = EGL_DONT_CARE;
        //EGLBoolean  bind_to_tex_rgba   = EGL_DONT_CARE;
        EGLenum     caveat             = EGL_DONT_CARE;
        EGLint      config_id          = EGL_DONT_CARE;
        EGLBoolean  native_renderable  = EGL_DONT_CARE;
        EGLint      native_visual_type = EGL_DONT_CARE;
        //EGLint      max_swap_interval  = EGL_DONT_CARE;
        //EGLint      min_swap_interval  = EGL_DONT_CARE;
        EGLint      trans_red_val      = EGL_DONT_CARE;
        EGLint      trans_green_val    = EGL_DONT_CARE;
        EGLint      trans_blue_val     = EGL_DONT_CARE;
        EGLenum     transparent_type   = EGL_NONE;
        // EGLint      buffer_size        = 0;
        EGLint      red_size           = 0;
        EGLint      green_size         = 0;
        EGLint      blue_size          = 0;
        EGLint      alpha_size         = 0;
        EGLint      depth_size         = 0;
        EGLint      frame_buffer_level = 0;
        EGLint      sample_buffers_num = 0;
        EGLint      samples_per_pixel  = 0;
        EGLint      stencil_size       = 0;
        EGLint      conformant         = 0;

        EGLBoolean  recordable_android = EGL_FALSE;
        EGLBoolean  framebuffer_target_android = EGL_DONT_CARE;

        EGLint luminance_size = 0;
        EGLint wanted_buffer_size = EGL_DONT_CARE;

        std::vector<EGLint> wanted_attribs;

    if(!EglValidate::noAttribs(attrib_list)) { //there are attribs
        int i = 0 ;
        bool hasConfigId = false;
        while(attrib_list[i] != EGL_NONE && !hasConfigId) {
#define CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(attrname) \
            CHOOSE_CONFIG_DLOG("EGL_BAD_ATTRIBUTE: " #attrname "defined as 0x%x", attrib_list[i+1]);

            if (attrib_list[i] != EGL_LEVEL &&
                attrib_list[i] != EGL_MATCH_NATIVE_PIXMAP &&
                attrib_list[i + 1] == EGL_DONT_CARE) {
                i+=2;
                continue;
            }

            switch(attrib_list[i]) {
            case EGL_MAX_PBUFFER_WIDTH:
            case EGL_MAX_PBUFFER_HEIGHT:
            case EGL_MAX_PBUFFER_PIXELS:
            case EGL_NATIVE_VISUAL_ID:
                break; //we dont care from those selection crateria
            case EGL_LEVEL:
                if(attrib_list[i+1] == EGL_DONT_CARE) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_LEVEL);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                frame_buffer_level = attrib_list[i+1];
                wanted_attribs.push_back(EGL_LEVEL);
                break;
            case EGL_BUFFER_SIZE:
                if(attrib_list[i+1] < 0) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_BUFFER_SIZE);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                wanted_attribs.push_back(EGL_BUFFER_SIZE);
                wanted_buffer_size = attrib_list[i + 1];
                break;
            case EGL_RED_SIZE:
                if(attrib_list[i+1] < 0) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_RED_SIZE);
                     RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                red_size = attrib_list[i+1];
                wanted_attribs.push_back(EGL_RED_SIZE);
                break;
            case EGL_GREEN_SIZE:
                if(attrib_list[i+1] < 0) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_GREEN_SIZE);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                green_size = attrib_list[i+1];
                wanted_attribs.push_back(EGL_GREEN_SIZE);
                break;
            case EGL_BLUE_SIZE:
                if(attrib_list[i+1] < 0) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_BLUE_SIZE);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                blue_size = attrib_list[i+1];
                wanted_attribs.push_back(EGL_BLUE_SIZE);
                break;
            case EGL_LUMINANCE_SIZE:
                if(attrib_list[i+1] < 0) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_LUMINANCE_SIZE);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                wanted_attribs.push_back(EGL_LUMINANCE_SIZE);
                luminance_size = attrib_list[i + 1];
                break;
            case EGL_ALPHA_SIZE:
                if(attrib_list[i+1] < 0) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_ALPHA_SIZE);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                alpha_size = attrib_list[i+1];
                wanted_attribs.push_back(EGL_ALPHA_SIZE);
                break;
            case EGL_ALPHA_MASK_SIZE:
                if(attrib_list[i+1] < 0) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_ALPHA_MASK_SIZE);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                wanted_attribs.push_back(EGL_ALPHA_MASK_SIZE);
                break;
            case EGL_BIND_TO_TEXTURE_RGB:
                if (attrib_list[i+1] != EGL_TRUE &&
                    attrib_list[i+1] != EGL_FALSE) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_BIND_TO_TEXTURE_RGB);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                wanted_attribs.push_back(EGL_BIND_TO_TEXTURE_RGB);
                //bind_to_tex_rgb = attrib_list[i+1];
                break;
            case EGL_BIND_TO_TEXTURE_RGBA:
                if (attrib_list[i+1] != EGL_TRUE &&
                    attrib_list[i+1] != EGL_FALSE) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_BIND_TO_TEXTURE_RGBA);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                wanted_attribs.push_back(EGL_BIND_TO_TEXTURE_RGBA);
                //bind_to_tex_rgba = attrib_list[i+1];
                break;
            case EGL_CONFIG_CAVEAT:
                if(attrib_list[i+1] != EGL_NONE &&
                   attrib_list[i+1] != EGL_SLOW_CONFIG &&
                   attrib_list[i+1] != EGL_NON_CONFORMANT_CONFIG) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_CONFIG_CAVEAT);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                caveat = attrib_list[i+1];
                wanted_attribs.push_back(EGL_CONFIG_CAVEAT);
                break;
            case EGL_CONFORMANT:
                conformant = attrib_list[i+1];
                wanted_attribs.push_back(EGL_CONFORMANT);
                break;
            case EGL_CONFIG_ID:
                if(attrib_list[i+1] < 0) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_CONFIG_ID);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                config_id = attrib_list[i+1];
                hasConfigId = true;
                wanted_attribs.push_back(EGL_CONFIG_ID);
                break;
            case EGL_DEPTH_SIZE:
                if(attrib_list[i+1] < 0) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_DEPTH_SIZE);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                depth_size = attrib_list[i+1];
                wanted_attribs.push_back(EGL_DEPTH_SIZE);
                break;
            case EGL_MAX_SWAP_INTERVAL:
                if(attrib_list[i+1] < 0) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_MAX_SWAP_INTERVAL);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                //max_swap_interval = attrib_list[i+1];
                wanted_attribs.push_back(EGL_MAX_SWAP_INTERVAL);
                break;
            case EGL_MIN_SWAP_INTERVAL:
                if(attrib_list[i+1] < 0) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_MIN_SWAP_INTERVAL);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                //min_swap_interval = attrib_list[i+1];
                wanted_attribs.push_back(EGL_MIN_SWAP_INTERVAL);
                break;
            case EGL_NATIVE_RENDERABLE:
                if (attrib_list[i+1] != EGL_TRUE &&
                    attrib_list[i+1] != EGL_FALSE) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_NATIVE_RENDERABLE);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                native_renderable = attrib_list[i+1];
                wanted_attribs.push_back(EGL_NATIVE_RENDERABLE);
                break;
            case EGL_RENDERABLE_TYPE:
                renderable_type = attrib_list[i+1];
                wanted_attribs.push_back(EGL_RENDERABLE_TYPE);
                break;
            case EGL_NATIVE_VISUAL_TYPE:
                native_visual_type = attrib_list[i+1];
                break;
                if(attrib_list[i+1] < 0 || attrib_list[i+1] > 1 ) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_NATIVE_VISUAL_TYPE);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                wanted_attribs.push_back(EGL_NATIVE_VISUAL_TYPE);
            case EGL_SAMPLE_BUFFERS:
                if(attrib_list[i+1] < 0) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_SAMPLE_BUFFERS);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                sample_buffers_num = attrib_list[i+1];
                wanted_attribs.push_back(EGL_SAMPLE_BUFFERS);
                break;
            case EGL_SAMPLES:
                if(attrib_list[i+1] < 0) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_SAMPLES);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                samples_per_pixel = attrib_list[i+1];
                wanted_attribs.push_back(EGL_SAMPLES);
                break;
            case EGL_STENCIL_SIZE:
                if(attrib_list[i+1] < 0) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_STENCIL_SIZE);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                stencil_size = attrib_list[i+1];
                wanted_attribs.push_back(EGL_STENCIL_SIZE);
                break;
            case EGL_SURFACE_TYPE:
                surface_type = attrib_list[i+1];
                wanted_attribs.push_back(EGL_SURFACE_TYPE);
                break;
            case EGL_TRANSPARENT_TYPE:
                if(attrib_list[i+1] != EGL_NONE &&
                   attrib_list[i+1] != EGL_TRANSPARENT_RGB ) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_TRANSPARENT_TYPE);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                transparent_type = attrib_list[i+1];
                wanted_attribs.push_back(EGL_TRANSPARENT_TYPE);
                break;
            case EGL_TRANSPARENT_RED_VALUE:
                trans_red_val = attrib_list[i+1];
                wanted_attribs.push_back(EGL_TRANSPARENT_RED_VALUE);
                break;
            case EGL_TRANSPARENT_GREEN_VALUE:
                trans_green_val = attrib_list[i+1];
                wanted_attribs.push_back(EGL_TRANSPARENT_GREEN_VALUE);
                break;
            case EGL_TRANSPARENT_BLUE_VALUE:
                trans_blue_val = attrib_list[i+1];
                wanted_attribs.push_back(EGL_TRANSPARENT_BLUE_VALUE);
                break;
            case EGL_COLOR_BUFFER_TYPE:
                if(attrib_list[i+1] != EGL_RGB_BUFFER &&
                   attrib_list[i+1] != EGL_LUMINANCE_BUFFER) {
                    CHOOSE_CONFIG_DLOG_BAD_ATTRIBUTE(EGL_COLOR_BUFFER_TYPE);
                    RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
                }
                wanted_attribs.push_back(EGL_COLOR_BUFFER_TYPE);
                break;
            case EGL_RECORDABLE_ANDROID:
                recordable_android = attrib_list[i+1];
                wanted_attribs.push_back(EGL_RECORDABLE_ANDROID);
                break;
            case EGL_FRAMEBUFFER_TARGET_ANDROID:
                framebuffer_target_android = attrib_list[i+1];
                wanted_attribs.push_back(EGL_FRAMEBUFFER_TARGET_ANDROID);
                break;
            default:
                CHOOSE_CONFIG_DLOG("EGL_BAD_ATTRIBUTE: Unknown attribute key 0x%x", attrib_list[i]);
                RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
            }
            i+=2;
        }
        if(hasConfigId) {
            EglConfig* pConfig = dpy->getConfig(config_id);
            if(pConfig) {
                if(configs) {
                    configs[0]  = static_cast<EGLConfig>(pConfig);
                }
                *num_config = 1;
                CHOOSE_CONFIG_DLOG("Using config id 0x%x. Return EGL_TRUE", config_id);
                return EGL_TRUE;
            } else {
                CHOOSE_CONFIG_DLOG("EGL_BAD_ATTRIBUTE: Using missing config id 0x%x", config_id);
                RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
            }
        }
    }
    EglConfig dummy(red_size,green_size,blue_size,alpha_size,caveat,conformant,depth_size,
                    frame_buffer_level,0,0,0,native_renderable,renderable_type,0,native_visual_type,
                    sample_buffers_num, samples_per_pixel,stencil_size,luminance_size,wanted_buffer_size,
                    surface_type,transparent_type,trans_red_val,trans_green_val,trans_blue_val,recordable_android, framebuffer_target_android,
                    NULL);
    for (size_t i = 0; i < wanted_attribs.size(); i++) {
        dummy.addWantedAttrib(wanted_attribs[i]);
    }
    *num_config = dpy->chooseConfigs(dummy,configs,config_size);
    CHOOSE_CONFIG_DLOG("eglChooseConfig: Success(EGL_TRUE). Num configs returned:%d", *num_config);

    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetConfigAttrib(EGLDisplay display, EGLConfig config,
                  EGLint attribute, EGLint *value) {
    VALIDATE_DISPLAY(display);
    VALIDATE_CONFIG(config);
    if(!EglValidate::confAttrib(attribute)){
        RETURN_ERROR(EGL_FALSE, EGL_BAD_ATTRIBUTE);
    }
    return cfg->getConfAttrib(attribute,value)? EGL_TRUE:EGL_FALSE;
}

EGLAPI EGLSurface EGLAPIENTRY eglCreateWindowSurface(EGLDisplay display, EGLConfig config,
                  EGLNativeWindowType win,
                  const EGLint *attrib_list) {
    VALIDATE_DISPLAY_RETURN(display,EGL_NO_SURFACE);
    VALIDATE_CONFIG_RETURN(config,EGL_NO_SURFACE);

    if(!(cfg->surfaceType() & EGL_WINDOW_BIT)) {
        RETURN_ERROR(EGL_NO_SURFACE,EGL_BAD_MATCH);
    }
    if(!dpy->nativeType()->isValidNativeWin(win)) {
        RETURN_ERROR(EGL_NO_SURFACE,EGL_BAD_NATIVE_WINDOW);
    }
    if(!EglValidate::noAttribs(attrib_list)) {
        RETURN_ERROR(EGL_NO_SURFACE,EGL_BAD_ATTRIBUTE);
    }
    if(EglWindowSurface::alreadyAssociatedWithConfig(win)) {
        RETURN_ERROR(EGL_NO_SURFACE,EGL_BAD_ALLOC);
    }

    emugl::Mutex::AutoLock mutex(s_eglLock);
    unsigned int width,height;
    if(!dpy->nativeType()->checkWindowPixelFormatMatch(
            win, cfg->nativeFormat(), &width, &height)) {
        RETURN_ERROR(EGL_NO_SURFACE,EGL_BAD_ALLOC);
    }
    SurfacePtr wSurface(new EglWindowSurface(dpy, win,cfg,width,height));
    if(!wSurface.get()) {
        RETURN_ERROR(EGL_NO_SURFACE,EGL_BAD_ALLOC);
    }
    return dpy->addSurface(wSurface);
}

EGLAPI EGLSurface EGLAPIENTRY eglCreatePbufferSurface(
        EGLDisplay display,
        EGLConfig config,
        const EGLint *attrib_list) {
    VALIDATE_DISPLAY_RETURN(display,EGL_NO_SURFACE);
    VALIDATE_CONFIG_RETURN(config,EGL_NO_SURFACE);
    if(!(cfg->surfaceType() & EGL_PBUFFER_BIT)) {
        RETURN_ERROR(EGL_NO_SURFACE,EGL_BAD_MATCH);
    }

    SurfacePtr pbSurface(new EglPbufferSurface(dpy,cfg));
    if(!pbSurface.get()) {
        RETURN_ERROR(EGL_NO_SURFACE,EGL_BAD_ALLOC);
    }

    if(!EglValidate::noAttribs(attrib_list)) { // There are attribs.
        int i = 0 ;
        while(attrib_list[i] != EGL_NONE) {
            if(!pbSurface->setAttrib(attrib_list[i],attrib_list[i+1])) {
                RETURN_ERROR(EGL_NO_SURFACE,EGL_BAD_ATTRIBUTE);
            }
            i+=2;
        }
    }

    EGLint width, height, largest, texTarget, texFormat;
    EglPbufferSurface* tmpPbSurfacePtr =
            static_cast<EglPbufferSurface*>(pbSurface.get());

    tmpPbSurfacePtr->getDim(&width, &height, &largest);
    tmpPbSurfacePtr->getTexInfo(&texTarget, &texFormat);

    if(!EglValidate::pbufferAttribs(width,
                                    height,
                                    texFormat == EGL_NO_TEXTURE,
                                    texTarget == EGL_NO_TEXTURE)) {
        //TODO: RETURN_ERROR(EGL_NO_SURFACE,EGL_BAD_VALUE); dont have bad_value
        RETURN_ERROR(EGL_NO_SURFACE,EGL_BAD_ATTRIBUTE);
    }

    EglOS::PbufferInfo pbinfo;

    pbinfo.width = width;
    pbinfo.height = height;
    pbinfo.largest = largest;
    pbinfo.target = texTarget;
    pbinfo.format = texFormat;

    tmpPbSurfacePtr->getAttrib(EGL_MIPMAP_TEXTURE, &pbinfo.hasMipmap);

    emugl::Mutex::AutoLock mutex(s_eglLock);
    EglOS::Surface* pb = dpy->nativeType()->createPbufferSurface(
            cfg->nativeFormat(), &pbinfo);
    if(!pb) {
        //TODO: RETURN_ERROR(EGL_NO_SURFACE,EGL_BAD_VALUE); dont have bad value
        RETURN_ERROR(EGL_NO_SURFACE,EGL_BAD_ATTRIBUTE);
    }

    tmpPbSurfacePtr->setNativePbuffer(pb);
    return dpy->addSurface(pbSurface);
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroySurface(EGLDisplay display, EGLSurface surface) {
    VALIDATE_DISPLAY(display);
    emugl::Mutex::AutoLock mutex(s_eglLock);
    SurfacePtr srfc = dpy->getSurface(surface);
    if(!srfc.get()) {
        RETURN_ERROR(EGL_FALSE,EGL_BAD_SURFACE);
    }
    g_eglInfo->markSurfaceForDestroy(dpy, surface);
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglQuerySurface(EGLDisplay display, EGLSurface surface,
               EGLint attribute, EGLint *value) {
   VALIDATE_DISPLAY(display);
   VALIDATE_SURFACE(surface,srfc);

   if(!srfc->getAttrib(attribute,value)) {
       RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
   }
   return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSurfaceAttrib(EGLDisplay display, EGLSurface surface,
                EGLint attribute, EGLint value) {
   VALIDATE_DISPLAY(display);
   VALIDATE_SURFACE(surface,srfc);
   if(!srfc->setAttrib(attribute,value)) {
       RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
   }
   return EGL_TRUE;
}

// eglCreateOrLoadContext is the implementation of eglCreateContext and
// eglLoadContext.
// |stream| is the snapshot file to load from when calling from eglLoadContext
// when |stream| is available, config and share group ID will be loaded from stream

static EGLContext eglCreateOrLoadContext(EGLDisplay display, EGLConfig config,
                EGLContext share_context,
                const EGLint *attrib_list,
                android::base::Stream *stream) {
    assert(share_context == EGL_NO_CONTEXT || stream == nullptr);
    VALIDATE_DISPLAY_RETURN(display,EGL_NO_CONTEXT);

    uint64_t shareGroupId = 0;
    EglConfig* cfg = nullptr;
    if (!stream) {
        cfg = dpy->getConfig(config);
        if (!cfg) return EGL_NO_CONTEXT;
    }

    EGLint major_version = 0;
    EGLint minor_version = 0;
    EGLint context_flags = 0;
    EGLint profile_mask = 0;
    EGLint reset_notification_strategy = 0;
    if(!EglValidate::noAttribs(attrib_list)) {
        int i = 0;
        while(attrib_list[i] != EGL_NONE) {
            EGLint attrib_val = attrib_list[i + 1];
            switch(attrib_list[i]) {
            case EGL_CONTEXT_MAJOR_VERSION_KHR:
                major_version = attrib_val;
                break;
            case EGL_CONTEXT_MINOR_VERSION_KHR:
                minor_version = attrib_val;
                break;
            case EGL_CONTEXT_FLAGS_KHR:
                if ((attrib_val | EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR) ||
                    (attrib_val | EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR)  ||
                    (attrib_val | EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR)) {
                    context_flags = attrib_val;
                } else {
                    fprintf(stderr, "%s: wrong context flags, return\n", __func__);
                    RETURN_ERROR(EGL_NO_CONTEXT,EGL_BAD_ATTRIBUTE);
                }
                break;
            case EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR:
                if ((attrib_val | EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR) ||
                    (attrib_val | EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR)) {
                    profile_mask = attrib_val;
                } else {
                    fprintf(stderr, "%s: wrong profile mask, return\n", __func__);
                    RETURN_ERROR(EGL_NO_CONTEXT,EGL_BAD_ATTRIBUTE);
                }
                break;
            case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_KHR:
                switch (attrib_val) {
                case EGL_NO_RESET_NOTIFICATION_KHR:
                case EGL_LOSE_CONTEXT_ON_RESET_KHR:
                    break;
                default:
                    fprintf(stderr, "%s: wrong reset notif strat, return\n", __func__);
                    RETURN_ERROR(EGL_NO_CONTEXT,EGL_BAD_ATTRIBUTE);
                }
                reset_notification_strategy = attrib_val;
                break;
            default:
                fprintf(stderr, "%s: unknown attrib 0x%x\n", __func__, attrib_list[i]);
                RETURN_ERROR(EGL_NO_CONTEXT,EGL_BAD_ATTRIBUTE);
            }
            i+=2;
        }
    }

    // TODO: Investigate these ignored flags and see which are needed
    (void)context_flags;
    (void)reset_notification_strategy;

    GLESVersion glesVersion;
    switch (major_version) {
     case 1:
         glesVersion = GLES_1_1;
         break;
     case 2:
         glesVersion = GLES_2_0;
         break;
     case 3:
         switch (minor_version) {
         case 0:
             glesVersion = GLES_3_0;
             break;
         case 1:
             glesVersion = GLES_3_1;
             break;
         default:
             RETURN_ERROR(EGL_NO_CONTEXT, EGL_BAD_ATTRIBUTE);
             break;
         }
         break;
     default:
         RETURN_ERROR(EGL_NO_CONTEXT, EGL_BAD_ATTRIBUTE);
         break;
    }

    const GLESiface* iface = g_eglInfo->getIface(glesVersion);
    GLEScontext* glesCtx = NULL;
    if(iface) {
        glesCtx = iface->createGLESContext(major_version, minor_version,
                dpy->getGlobalNameSpace(), stream);
    } else { // there is no interface for this gles version
                RETURN_ERROR(EGL_NO_CONTEXT,EGL_BAD_ATTRIBUTE);
    }

    if(share_context != EGL_NO_CONTEXT) {
        ContextPtr sharedCtxPtr = dpy->getContext(share_context);
        if(!sharedCtxPtr.get()) {
            RETURN_ERROR(EGL_NO_CONTEXT,EGL_BAD_CONTEXT);
        }
        shareGroupId = sharedCtxPtr->getShareGroup()->getId();
        assert(shareGroupId);
    }

    emugl::Mutex::AutoLock mutex(s_eglLock);

    ContextPtr ctx(new EglContext(dpy, shareGroupId, cfg,
                                  glesCtx, glesVersion,
                                  profile_mask,
                                  dpy->getManager(glesVersion),
                                  stream));
    if(ctx->nativeType()) {
        return dpy->addContext(ctx);
    } else {
        iface->deleteGLESContext(glesCtx);
    }

    return EGL_NO_CONTEXT;
}

EGLAPI EGLContext EGLAPIENTRY eglCreateContext(EGLDisplay display, EGLConfig config,
                EGLContext share_context,
                const EGLint *attrib_list) {
    return eglCreateOrLoadContext(display, config, share_context, attrib_list, nullptr);
}

EGLAPI EGLContext EGLAPIENTRY eglLoadContext(EGLDisplay display, const EGLint *attrib_list,
                                             android::base::Stream *stream) {
    return eglCreateOrLoadContext(display, (EGLConfig)0, (EGLContext)0, attrib_list, stream);
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroyContext(EGLDisplay display, EGLContext context) {
    VALIDATE_DISPLAY(display);
    VALIDATE_CONTEXT(context);

    emugl::Mutex::AutoLock mutex(s_eglLock);
    dpy->removeContext(context);
    return EGL_TRUE;
}

static void sGetPbufferSurfaceGLProperties(
        EglPbufferSurface* surface,
        EGLint* width, EGLint* height, GLint* multisamples,
        GLint* colorFormat, GLint* depthStencilFormat) {

    assert(width);
    assert(height);
    assert(multisamples);
    assert(colorFormat);
    assert(depthStencilFormat);

    EGLint r, g, b, a, d, s;
    surface->getAttrib(EGL_WIDTH, width);
    surface->getAttrib(EGL_HEIGHT, height);
    surface->getAttrib(EGL_RED_SIZE, &r);
    surface->getAttrib(EGL_GREEN_SIZE, &g);
    surface->getAttrib(EGL_BLUE_SIZE, &b);
    surface->getAttrib(EGL_ALPHA_SIZE, &a);
    surface->getAttrib(EGL_DEPTH_SIZE, &d);
    surface->getAttrib(EGL_STENCIL_SIZE, &s);
    surface->getAttrib(EGL_SAMPLES, multisamples);

    // Currently supported: RGBA8888/RGB888/RGB565/RGBA4/RGB5A1
    if (r == 8 && g == 8 && b == 8 && a == 8) {
        *colorFormat = GL_RGBA8;
    } else if (r == 8 && g == 8 && b == 8 && a == 0) {
        *colorFormat = GL_RGB8;
    } else if (r == 5 && g == 6 && b == 5 && a == 0) {
        *colorFormat = GL_RGB565;
    } else if (r == 4 && g == 4 && b == 4 && a == 4) {
        *colorFormat = GL_RGBA4;
    } else if (r == 5 && g == 5 && b == 5 && a == 1) {
        *colorFormat = GL_RGB5_A1;
    } else {
        GFXSTREAM_ABORT(FatalError(ABORT_REASON_OTHER))
            << "invalid color format R" << r << "G" << g << "B" << b << "A" << a;
    }

    // Blanket provide 24/8 depth/stencil format for now.
    *depthStencilFormat = GL_DEPTH24_STENCIL8;

    // TODO: Support more if necessary, or even restrict
    // EGL configs from host display to only these ones.
}

EGLAPI EGLBoolean EGLAPIENTRY eglMakeCurrent(EGLDisplay display,
                                             EGLSurface draw,
                                             EGLSurface read,
                                             EGLContext context) {
    VALIDATE_DISPLAY(display);

    bool releaseContext = EglValidate::releaseContext(context, read, draw);
    if(!releaseContext && EglValidate::badContextMatch(context, read, draw)) {
        RETURN_ERROR(EGL_FALSE, EGL_BAD_MATCH);
    }

    ThreadInfo* thread = getThreadInfo();
    ContextPtr prevCtx = thread->eglContext;

    if(releaseContext) { //releasing current context
       if(prevCtx.get()) {
           g_eglInfo->getIface(prevCtx->version())->flush();
           if(!dpy->nativeType()->makeCurrent(NULL,NULL,NULL)) {
               RETURN_ERROR(EGL_FALSE,EGL_BAD_ACCESS);
           }
           thread->updateInfo(ContextPtr(),dpy,NULL,ShareGroupPtr(),dpy->getManager(prevCtx->version()));
       }
    } else { //assining new context
        VALIDATE_CONTEXT(context);
        VALIDATE_SURFACE(draw,newDrawSrfc);
        VALIDATE_SURFACE(read,newReadSrfc);

        EglSurface* newDrawPtr = newDrawSrfc.get();
        EglSurface* newReadPtr = newReadSrfc.get();
        ContextPtr  newCtx     = ctx;

        if (newCtx.get() && prevCtx.get()) {
            if (newCtx.get() == prevCtx.get()) {
                if (newDrawPtr == prevCtx->draw().get() &&
                    newReadPtr == prevCtx->read().get()) {
                    // nothing to do
                    return EGL_TRUE;
                }
            }
            else {
                // Make sure previous context is detached from surfaces
                releaseContext = true;
            }
        }

        //surfaces compatibility check
        if(!((*ctx->getConfig()).compatibleWith((*newDrawPtr->getConfig()))) ||
           !((*ctx->getConfig()).compatibleWith((*newReadPtr->getConfig())))) {
            RETURN_ERROR(EGL_FALSE,EGL_BAD_MATCH);
        }

         EglOS::Display* nativeDisplay = dpy->nativeType();
         EglOS::Surface* nativeRead = newReadPtr->native();
         EglOS::Surface* nativeDraw = newDrawPtr->native();
        //checking native window validity
        if(newReadPtr->type() == EglSurface::WINDOW &&
                !nativeDisplay->isValidNativeWin(nativeRead)) {
            RETURN_ERROR(EGL_FALSE,EGL_BAD_NATIVE_WINDOW);
        }
        if(newDrawPtr->type() == EglSurface::WINDOW &&
                !nativeDisplay->isValidNativeWin(nativeDraw)) {
            RETURN_ERROR(EGL_FALSE,EGL_BAD_NATIVE_WINDOW);
        }

        if(prevCtx.get()) {
            g_eglInfo->getIface(prevCtx->version())->flush();
        }

        {
            emugl::Mutex::AutoLock mutex(s_eglLock);
            if (!dpy->nativeType()->makeCurrent(
                        newReadPtr->native(),
                        newDrawPtr->native(),
                        newCtx->nativeType())) {
                RETURN_ERROR(EGL_FALSE,EGL_BAD_ACCESS);
            }
            //TODO: handle the following errors
            // EGL_BAD_CURRENT_SURFACE , EGL_CONTEXT_LOST  , EGL_BAD_ACCESS

            thread->updateInfo(newCtx,dpy,newCtx->getGlesContext(),newCtx->getShareGroup(),dpy->getManager(newCtx->version()));
            newCtx->setSurfaces(newReadSrfc,newDrawSrfc);
            g_eglInfo->getIface(newCtx->version())->initContext(newCtx->getGlesContext(),newCtx->getShareGroup());
            g_eglInfo->sweepDestroySurfaces();
        }

        if (newDrawPtr->type() == EglSurface::PBUFFER &&
            newReadPtr->type() == EglSurface::PBUFFER) {

            EglPbufferSurface* tmpPbSurfacePtr =
                static_cast<EglPbufferSurface*>(newDrawPtr);
            EglPbufferSurface* tmpReadPbSurfacePtr =
                static_cast<EglPbufferSurface*>(newReadPtr);

            EGLint width, height, readWidth, readHeight;
            GLint colorFormat, depthStencilFormat, multisamples;
            GLint readColorFormat, readDepthStencilFormat, readMultisamples;

            sGetPbufferSurfaceGLProperties(
                    tmpPbSurfacePtr,
                    &width, &height,
                    &multisamples,
                    &colorFormat, &depthStencilFormat);

            sGetPbufferSurfaceGLProperties(
                    tmpReadPbSurfacePtr,
                    &readWidth, &readHeight,
                    &readMultisamples,
                    &readColorFormat, &readDepthStencilFormat);

            newCtx->getGlesContext()->initDefaultFBO(
                    width, height,
                    colorFormat, depthStencilFormat, multisamples,
                    &tmpPbSurfacePtr->glRboColor,
                    &tmpPbSurfacePtr->glRboDepth,
                    readWidth, readHeight,
                    readColorFormat, readDepthStencilFormat, readMultisamples,
                    &tmpReadPbSurfacePtr->glRboColor,
                    &tmpReadPbSurfacePtr->glRboDepth);
        }

        // Initialize the GLES extension function table used in
        // eglGetProcAddress for the context's GLES version if not
        // yet initialized. We initialize it here to make sure we call the
        // GLES getProcAddress after when a context is bound.
        g_eglInfo->initClientExtFuncTable(newCtx->version());
    }

    // release previous context surface binding
    if(prevCtx.get() && releaseContext) {
        prevCtx->setSurfaces(SurfacePtr(),SurfacePtr());
    }

    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglQueryContext(EGLDisplay display, EGLContext context,
               EGLint attribute, EGLint *value) {
    VALIDATE_DISPLAY(display);
    VALIDATE_CONTEXT(context);

    if(!ctx->getAttrib(attribute,value)){
        RETURN_ERROR(EGL_FALSE,EGL_BAD_ATTRIBUTE);
    }
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSwapBuffers(EGLDisplay display, EGLSurface surface) {
    VALIDATE_DISPLAY(display);
    VALIDATE_SURFACE(surface,Srfc);
    ThreadInfo* thread        = getThreadInfo();
    ContextPtr currentCtx    = thread->eglContext;


    //if surface not window return
    if(Srfc->type() != EglSurface::WINDOW){
        RETURN_ERROR(EGL_TRUE,EGL_SUCCESS);
    }

    if(!currentCtx.get() || !currentCtx->usingSurface(Srfc) ||
            !dpy->nativeType()->isValidNativeWin(Srfc.get()->native())) {
        RETURN_ERROR(EGL_FALSE,EGL_BAD_SURFACE);
    }

    dpy->nativeType()->swapBuffers(Srfc->native());
    return EGL_TRUE;
}

EGLAPI EGLContext EGLAPIENTRY eglGetCurrentContext(void) {
    MEM_TRACE("EMUGL");
    emugl::Mutex::AutoLock mutex(s_eglLock);
    ThreadInfo* thread = getThreadInfo();
    EglDisplay* dpy    = static_cast<EglDisplay*>(thread->eglDisplay);
    ContextPtr  ctx    = thread->eglContext;
    if(dpy && ctx.get()){
        // This double check is required because a context might still be current after it is destroyed - in which case
        // its handle should be invalid, that is EGL_NO_CONTEXT should be returned even though the context is current
        EGLContext c = (EGLContext)SafePointerFromUInt(ctx->getHndl());
        if(dpy->getContext(c).get())
        {
            return c;
        }
    }
    return EGL_NO_CONTEXT;
}

EGLAPI EGLSurface EGLAPIENTRY eglGetCurrentSurface(EGLint readdraw) {
    MEM_TRACE("EMUGL");
    emugl::Mutex::AutoLock mutex(s_eglLock);
    if (!EglValidate::surfaceTarget(readdraw)) {
        return EGL_NO_SURFACE;
    }

    ThreadInfo* thread = getThreadInfo();
    EglDisplay* dpy    = static_cast<EglDisplay*>(thread->eglDisplay);
    ContextPtr  ctx    = thread->eglContext;

    if(dpy && ctx.get()) {
        SurfacePtr surface = readdraw == EGL_READ ? ctx->read() : ctx->draw();
        if(surface.get())
        {
            // This double check is required because a surface might still be
            // current after it is destroyed - in which case its handle should
            // be invalid, that is EGL_NO_SURFACE should be returned even
            // though the surface is current.
            EGLSurface s = (EGLSurface)SafePointerFromUInt(surface->getHndl());
            surface = dpy->getSurface(s);
            if(surface.get())
            {
                return s;
            }
        }
    }
    return EGL_NO_SURFACE;
}

EGLAPI EGLDisplay EGLAPIENTRY eglGetCurrentDisplay(void) {
    MEM_TRACE("EMUGL");
    ThreadInfo* thread = getThreadInfo();
    return (thread->eglContext.get()) ? thread->eglDisplay : EGL_NO_DISPLAY;
}

EGLAPI EGLBoolean EGLAPIENTRY eglBindAPI(EGLenum api) {
    MEM_TRACE("EMUGL");
    if(!EglValidate::supportedApi(api)) {
        RETURN_ERROR(EGL_FALSE,EGL_BAD_PARAMETER);
    }
    CURRENT_THREAD();
    tls_thread->setApi(api);
    return EGL_TRUE;
}

EGLAPI EGLenum EGLAPIENTRY eglQueryAPI(void) {
    MEM_TRACE("EMUGL");
    CURRENT_THREAD();
    return tls_thread->getApi();
}

EGLAPI EGLBoolean EGLAPIENTRY eglReleaseThread(void) {
    MEM_TRACE("EMUGL");
    ThreadInfo* thread  = getThreadInfo();
    EglDisplay* dpy     = static_cast<EglDisplay*>(thread->eglDisplay);
    return translator::egl::eglMakeCurrent(dpy,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
}

EGLAPI void* EGLAPIENTRY
eglGetProcAddress(const char *procname){
    __eglMustCastToProperFunctionPointerType retVal = NULL;

    if(!strncmp(procname,"egl",3)) { //EGL proc
        for(int i=0;i < s_eglExtensionsSize;i++){
            if(strcmp(procname,s_eglExtensions[i].name) == 0){
                retVal = s_eglExtensions[i].address;
                break;
            }
        }
    }
    else {
        // Look at the clientAPI (GLES) supported extension
        // function table.
        retVal = ClientAPIExts::getProcAddress(procname);
    }
    return (void*)retVal;
}

/************************** KHR IMAGE *************************************************************/
ImagePtr getEGLImage(unsigned int imageId)
{
    ThreadInfo* thread  = getThreadInfo();
    EglDisplay* dpy     = static_cast<EglDisplay*>(thread->eglDisplay);
    ContextPtr  ctx     = thread->eglContext;
    if (ctx.get()) {
        const GLESiface* iface = g_eglInfo->getIface(GLES_2_0);
        return dpy->getImage(reinterpret_cast<EGLImageKHR>(imageId),
                iface->restoreTexture);
    }
    return nullptr;
}

EGLAPI EGLImageKHR EGLAPIENTRY eglCreateImageKHR(EGLDisplay display, EGLContext context, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list)
{
    VALIDATE_DISPLAY(display);
    VALIDATE_CONTEXT(context);

	if (target != EGL_GL_TEXTURE_2D_KHR) {
		// Create image from underlying and add to registry
		EGLImage image = dpy->createNativeImage(dpy->getHostDriverDisplay(), 0, target, buffer, attrib_list);

		if (image == EGL_NO_IMAGE_KHR) {
			return EGL_NO_IMAGE_KHR;
		}

		ImagePtr img( new EglImage() );
		img->isNative = true;
		img->nativeImage = image;
		img->width = 0;
		img->height = 0;
		if (attrib_list) {
			const EGLint* current = attrib_list;
			while (EGL_NONE != *current) {
				switch (*current) {
					case EGL_WIDTH:
						img->width = current[1];
						break;
					case EGL_HEIGHT:
						img->height = current[1];
						break;
					case EGL_LINUX_DRM_FOURCC_EXT:
					    // TODO: Translate drm fourcc to internal format
						// img->fourcc = current[1];
						break;
				}
				current += 2;
			}
		}
		return dpy->addImageKHR(img);
	}

    ThreadInfo* thread  = getThreadInfo();
    ShareGroupPtr sg = thread->shareGroup;
    if (sg.get() != NULL) {
        NamedObjectPtr globalTexObject = sg->getNamedObject(NamedObjectType::TEXTURE,
                                                            SafeUIntFromPointer(buffer));
        if (!globalTexObject) return EGL_NO_IMAGE_KHR;

        ImagePtr img( new EglImage() );
        if (img.get() != NULL) {
            auto objData = sg->getObjectData(
                    NamedObjectType::TEXTURE, SafeUIntFromPointer(buffer));
            if (!objData) return EGL_NO_IMAGE_KHR;

            TextureData *texData = (TextureData *)objData;
            if(!texData->width || !texData->height) return EGL_NO_IMAGE_KHR;
            img->width = texData->width;
            img->height = texData->height;
            img->border = texData->border;
            img->internalFormat = texData->internalFormat;
            img->globalTexObj = globalTexObject;
            img->format = texData->format;
            img->type = texData->type;
            img->texStorageLevels = texData->texStorageLevels;
            img->saveableTexture = texData->getSaveableTexture();
            img->needRestore = false;
            img->sync = nullptr;
            return dpy->addImageKHR(img);
        }
    }

    return EGL_NO_IMAGE_KHR;
}


EGLAPI EGLBoolean EGLAPIENTRY eglDestroyImageKHR(EGLDisplay display, EGLImageKHR image)
{
    VALIDATE_DISPLAY(display);
    unsigned int imagehndl = SafeUIntFromPointer(image);
    ImagePtr img = getEGLImage(imagehndl);

    if (!img) return EGL_FALSE;

    const GLESiface* iface = g_eglInfo->getIface(GLES_2_0);

    if (img->sync) {
        iface->deleteSync((GLsync)img->sync);
        img->sync = nullptr;
    }

    if (img->isNative && !img->isImported) {
        dpy->destroyNativeImage(dpy->getHostDriverDisplay(), img->nativeImage);
    }

    return dpy->destroyImageKHR(image) ? EGL_TRUE:EGL_FALSE;
}


EGLAPI EGLSyncKHR EGLAPIENTRY eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint* attrib_list) {
    MEM_TRACE("EMUGL");
    // swiftshader_indirect used to have a bug with eglCreateSyncKHR
    // but it seems to have been fixed now.
    // BUG: 65587659
    // BUG: 246740239
    if (!g_eglInfo->isEgl2EglSyncSafeToUse()) {
        return (EGLSyncKHR)0x42;
    }

    const GLESiface* iface = g_eglInfo->getIface(GLES_2_0);
    GLsync res = iface->fenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    return (EGLSyncKHR)res;
}

EGLAPI EGLint EGLAPIENTRY eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout) {
    MEM_TRACE("EMUGL");
    emugl::Mutex::AutoLock mutex(s_eglLock);
    if (!g_eglInfo->isEgl2EglSyncSafeToUse()) {
        return EGL_CONDITION_SATISFIED_KHR;
    }
    const GLESiface* iface = g_eglInfo->getIface(GLES_2_0);
    GLenum gl_wait_result =
        iface->clientWaitSync((GLsync)sync, GL_SYNC_FLUSH_COMMANDS_BIT, timeout);
    EGLint egl_wait_result;

    switch (gl_wait_result) {
    case GL_ALREADY_SIGNALED:
    case GL_CONDITION_SATISFIED:
        egl_wait_result = EGL_CONDITION_SATISFIED_KHR;
        break;
    case GL_TIMEOUT_EXPIRED:
        egl_wait_result = EGL_TIMEOUT_EXPIRED_KHR;
        break;
    case GL_WAIT_FAILED:
        egl_wait_result = EGL_FALSE;
        break;
    default:
        egl_wait_result = EGL_CONDITION_SATISFIED_KHR;
    }
    return egl_wait_result;
}

EGLAPI EGLBoolean EGLAPIENTRY eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync) {
    MEM_TRACE("EMUGL");
    if (!g_eglInfo->isEgl2EglSyncSafeToUse()) {
        return EGL_TRUE;
    }
    const GLESiface* iface = g_eglInfo->getIface(GLES_2_0);
    iface->deleteSync((GLsync)sync);
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglGetSyncAttribKHR(
    EGLDisplay dpy, EGLSyncKHR sync,
    EGLint attribute, EGLint *value) {
    MEM_TRACE("EMUGL");

    if (!g_eglInfo->isEgl2EglSyncSafeToUse()) {
        switch (attribute) {
            case EGL_SYNC_TYPE_KHR:
                *value = EGL_SYNC_FENCE_KHR;
                break;
            case EGL_SYNC_CONDITION_KHR:
                *value = EGL_SYNC_PRIOR_COMMANDS_COMPLETE_KHR;
                break;
            case EGL_SYNC_STATUS_KHR: {
                *value = EGL_SIGNALED_KHR;
                break;
            default:
                return EGL_FALSE;
            }
        }
        return EGL_TRUE;
    }

    switch (attribute) {
        // Guest doesn't care about sync type (handled in guest),
        // but host side might care
        case EGL_SYNC_TYPE_KHR:
            *value = EGL_SYNC_FENCE_KHR;
            break;
        case EGL_SYNC_CONDITION_KHR:
            *value = EGL_SYNC_PRIOR_COMMANDS_COMPLETE_KHR;
            break;
        case EGL_SYNC_STATUS_KHR: {
            const GLESiface* iface = g_eglInfo->getIface(GLES_2_0);
            GLint status = -1;
            iface->getSynciv((GLsync)sync, GL_SYNC_STATUS, sizeof(GLint), nullptr, &status);
            switch (status) {
                case GL_UNSIGNALED:
                    *value = EGL_UNSIGNALED_KHR;
                    break;
                case GL_SIGNALED:
                    *value = EGL_SIGNALED_KHR;
                    break;
                default:
                    // error, return EGL_FALSE
                    return EGL_FALSE;
            }
            break;
        }
        default:
            return EGL_FALSE;
    }

    return EGL_TRUE;
}

EGLAPI EGLint EGLAPIENTRY eglGetMaxGLESVersion(EGLDisplay display) {
    // 0: es2 1: es3.0 2: es3.1 3: es3.2
    VALIDATE_DISPLAY_RETURN(display, 0 /* gles2 */);
    return (EGLint)dpy->getMaxGlesVersion();
}

EGLAPI EGLint EGLAPIENTRY eglWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags) {
    MEM_TRACE("EMUGL");
    if (!g_eglInfo->isEgl2EglSyncSafeToUse()) {
        return EGL_TRUE;
    }
    const GLESiface* iface = g_eglInfo->getIface(GLES_2_0);
    iface->waitSync((GLsync)sync, 0, -1);
    return EGL_TRUE;
}

EGLAPI void EGLAPIENTRY eglBlitFromCurrentReadBufferANDROID(EGLDisplay dpy, EGLImageKHR image) {
    MEM_TRACE("EMUGL");
    const GLESiface* iface = g_eglInfo->getIface(GLES_2_0);
    iface->blitFromCurrentReadBufferANDROID((GLeglImageOES)image);
}

// Creates a fence checkpoint for operations that have happened to |image|.
// Other users of |image| can choose to wait on the resulting return fence so
// that operations on |image| occur in the correct order on the GPU.  For
// example, we might render some objects or upload image data to |image| in
// Thread A, and then in Thread B, read the results. It is not guaranteed that
// the write operations on |image| have finished on the GPU when we start
// reading, so we call eglSetImageFenceANDROID at the end of writing operations
// in Thread A, and then wait on the fence in Thread B.
EGLAPI void* EGLAPIENTRY eglSetImageFenceANDROID(EGLDisplay dpy, EGLImageKHR image) {
    MEM_TRACE("EMUGL");
    unsigned int imagehndl = SafeUIntFromPointer(image);
    ImagePtr img = getEGLImage(imagehndl);
    const GLESiface* iface = g_eglInfo->getIface(GLES_2_0);
    if (!img) return iface->fenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);


    if (img->sync) {
        iface->deleteSync((GLsync)img->sync);
        img->sync = nullptr;
    }

    GLsync res = iface->fenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    iface->flush();
    img->sync = res;
    return (void*)res;
}

EGLAPI void EGLAPIENTRY eglWaitImageFenceANDROID(EGLDisplay dpy, void* fence) {
    MEM_TRACE("EMUGL");
    const GLESiface* iface = g_eglInfo->getIface(GLES_2_0);
    iface->waitSync((GLsync)fence, 0, -1);
}

EGLAPI void EGLAPIENTRY eglAddLibrarySearchPathANDROID(const char* path) {
    MEM_TRACE("EMUGL");
    emugl::SharedLibrary::addLibrarySearchPath(path);
}

EGLAPI EGLBoolean EGLAPIENTRY eglQueryVulkanInteropSupportANDROID(void) {
    MEM_TRACE("EMUGL");
    const GLESiface* iface = g_eglInfo->getIface(GLES_2_0);
    return iface->vulkanInteropSupported() ? EGL_TRUE : EGL_FALSE;
}

/*********************************************************************************/

EGLAPI EGLBoolean EGLAPIENTRY eglPreSaveContext(EGLDisplay display, EGLContext contex, EGLStream stream) {
    const GLESiface* iface = g_eglInfo->getIface(GLES_2_0);
    assert(iface->saveTexture);
    if (!iface || !iface->saveTexture) return EGL_TRUE;
    VALIDATE_DISPLAY(display);
    VALIDATE_CONTEXT(contex);
    ctx->getShareGroup()->preSave(dpy->getGlobalNameSpace());
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSaveContext(EGLDisplay display, EGLContext contex, EGLStream stream) {
    VALIDATE_DISPLAY(display);
    VALIDATE_CONTEXT(contex);
    ctx->onSave((android::base::Stream*)stream);
    return EGL_TRUE;
}

EGLAPI EGLContext EGLAPIENTRY eglLoadContext(EGLDisplay display, const EGLint *attrib_list, EGLStream stream) {
    return eglCreateOrLoadContext(display, (EGLConfig)0, EGL_NO_CONTEXT, attrib_list, (android::base::Stream*)stream);
}

EGLAPI EGLBoolean EGLAPIENTRY eglPostSaveContext(EGLDisplay display, EGLContext context, EGLStream stream) {
    VALIDATE_DISPLAY(display);
    VALIDATE_CONTEXT(context);
    ctx->postSave((android::base::Stream*)stream);
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSaveConfig(EGLDisplay display,
        EGLConfig config, EGLStream stream) {
    VALIDATE_DISPLAY(display);
    VALIDATE_CONFIG(config);
    android::base::Stream* stm = static_cast<android::base::Stream*>(stream);
    stm->putBe32(cfg->id());
    return EGL_TRUE;
}

EGLAPI EGLConfig EGLAPIENTRY eglLoadConfig(EGLDisplay display, EGLStream stream) {
    VALIDATE_DISPLAY(display);
    android::base::Stream* stm = static_cast<android::base::Stream*>(stream);
    EGLint cfgId = stm->getBe32();
    EglConfig* cfg = dpy->getConfig(cfgId);
    if (!cfg) {
        dwarning("EGL config mismatch, fallback to default configs");
        cfg = dpy->getDefaultConfig();
    }
    return static_cast<EGLConfig>(cfg);
}

EGLAPI EGLBoolean EGLAPIENTRY eglSaveAllImages(EGLDisplay display,
                                               EGLStream stream,
                                               const void* textureSaver) {
    const GLESiface* iface = g_eglInfo->getIface(GLES_2_0);
    assert(iface->saveTexture);
    if (!iface || !iface->saveTexture)
        return true;
    VALIDATE_DISPLAY(display);
    android::base::Stream* stm = static_cast<android::base::Stream*>(stream);
    iface->preSaveTexture();
    dpy->onSaveAllImages(
            stm,
            *static_cast<const android::snapshot::ITextureSaverPtr*>(textureSaver),
            iface->saveTexture,
            iface->restoreTexture);
    iface->postSaveTexture();
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglLoadAllImages(EGLDisplay display,
                                               EGLStream stream,
                                               const void* textureLoader) {
    const GLESiface* iface = g_eglInfo->getIface(GLES_2_0);
    assert(iface->createTexture);
    if (!iface || !iface->createTexture)
        return true;
    VALIDATE_DISPLAY(display);
    android::base::Stream* stm = static_cast<android::base::Stream*>(stream);
    dpy->onLoadAllImages(
            stm,
            *static_cast<const android::snapshot::ITextureLoaderPtr*>(textureLoader),
            iface->createTexture);
    return EGL_TRUE;
}

EGLAPI EGLBoolean EGLAPIENTRY eglPostLoadAllImages(EGLDisplay display, EGLStream stream) {
    VALIDATE_DISPLAY(display);
    android::base::Stream* stm = static_cast<android::base::Stream*>(stream);
    dpy->postLoadAllImages(stm);
    return true;
}

EGLAPI void EGLAPIENTRY eglUseOsEglApi(EGLBoolean enable) {
    MEM_TRACE("EMUGL");
    EglGlobalInfo::setEgl2Egl(enable);
    EglGlobalInfo::setEgl2EglSyncSafeToUse(EGL_TRUE);
}

EGLAPI void EGLAPIENTRY eglSetMaxGLESVersion(EGLint version) {
    MEM_TRACE("EMUGL");
    // The "version" here follows the convention of eglGetMaxGLESVesion
    // 0: es2 1: es3.0 2: es3.1 3: es3.2
    GLESVersion glesVersion = GLES_2_0;
    switch (version) {
    case 0:
        glesVersion = GLES_2_0;
        break;
    case 1:
        glesVersion = GLES_3_0;
        break;
    case 2:
    case 3: // TODO: GLES 3.2 support?
        glesVersion = GLES_3_1;
        break;
    }

    // If egl2egl, set internal gles version to 3 as
    // that is what we use (EglOsApi_egl.cpp)
    if (EglGlobalInfo::isEgl2Egl()) {
        glesVersion = GLES_3_0;
    }

    if (g_eglInfo->getIface(GLES_1_1)) {
        g_eglInfo->getIface(GLES_1_1)->setMaxGlesVersion(glesVersion);
    }
    g_eglInfo->getIface(GLES_2_0)->setMaxGlesVersion(glesVersion);
}

EGLAPI void EGLAPIENTRY eglFillUsages(void* usages) {
    MEM_TRACE("EMUGL");
    // TODO: Figure out better usage metrics interface
    // that doesn't require linking protobuf into Translator
    // if (g_eglInfo->getIface(GLES_1_1) &&
    //         g_eglInfo->getIface(GLES_1_1)->fillGLESUsages) {
    //     g_eglInfo->getIface(GLES_1_1)->fillGLESUsages(
    //         (android_studio::EmulatorGLESUsages*)usages);
    // }
    // if (g_eglInfo->getIface(GLES_2_0) &&
    //         g_eglInfo->getIface(GLES_2_0)->fillGLESUsages) {
    //     g_eglInfo->getIface(GLES_2_0)->fillGLESUsages(
    //         (android_studio::EmulatorGLESUsages*)usages);
    // }
}

EGLAPI EGLDisplay EGLAPIENTRY eglGetNativeDisplayANDROID(EGLDisplay display) {
    VALIDATE_DISPLAY_RETURN(display, (EGLDisplay)0);
    return dpy->getHostDriverDisplay();
}

EGLAPI EGLContext EGLAPIENTRY eglGetNativeContextANDROID(EGLDisplay display, EGLContext context) {
    VALIDATE_DISPLAY_RETURN(display, (EGLContext)0);
    VALIDATE_CONTEXT_RETURN(context, (EGLContext)0);
    return dpy->getNativeContext(context);
}

EGLAPI EGLImage EGLAPIENTRY eglGetNativeImageANDROID(EGLDisplay display, EGLImage image) {
    VALIDATE_DISPLAY_RETURN(display, (EGLImage)0);
    unsigned int imagehndl = SafeUIntFromPointer(image);
    ImagePtr img = getEGLImage(imagehndl);
    if (!img || !img->isNative) return (EGLImage)0;
    return img->nativeImage;
}

EGLAPI EGLBoolean EGLAPIENTRY eglSetImageInfoANDROID(EGLDisplay display, EGLImage image, EGLint width, EGLint height, EGLint internalFormat) {
    VALIDATE_DISPLAY_RETURN(display, EGL_FALSE);
    unsigned int imagehndl = SafeUIntFromPointer(image);
    ImagePtr img = getEGLImage(imagehndl);
    if (!img) return EGL_FALSE;

    img->width = width;
    img->height = height;
    img->internalFormat = internalFormat;

    return EGL_TRUE;
}

EGLImage eglImportImageANDROID(EGLDisplay display, EGLImage nativeImage) {
    VALIDATE_DISPLAY_RETURN(display, (EGLImage)0);
	ImagePtr img( new EglImage() );
	img->isNative = true;
	img->isImported = true;
	img->nativeImage = nativeImage;
    return dpy->addImageKHR(img);
}

static const GLint kAuxiliaryContextAttribsCompat[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
};

static const GLint kAuxiliaryContextAttribsCore[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
    EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
    EGL_NONE
};

#define NAMESPACED_EGL(f) translator::egl::f

static bool createAndBindAuxiliaryContext(EGLContext* context_out, EGLSurface* surface_out) {
    // create the context
    EGLDisplay dpy = NAMESPACED_EGL(eglGetDisplay)(EGL_DEFAULT_DISPLAY);

    NAMESPACED_EGL(eglBindAPI)(EGL_OPENGL_ES_API);

    static const GLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT, EGL_NONE };

    EGLConfig config;
    int numConfigs;
    if (!NAMESPACED_EGL(eglChooseConfig)(dpy, configAttribs, &config, 1, &numConfigs) ||
        numConfigs == 0) {
        fprintf(stderr, "%s: could not find gles 2 config!\n", __func__);
        return false;
    }

    static const EGLint pbufAttribs[] =
        { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    EGLSurface surface = eglCreatePbufferSurface(dpy, config, pbufAttribs);
    if (!surface) {
        fprintf(stderr, "%s: could not create surface\n", __func__);
        return false;
    }

    EGLContext context =
        NAMESPACED_EGL(eglCreateContext)(dpy, config, EGL_NO_CONTEXT,
            isCoreProfile() ? kAuxiliaryContextAttribsCore :
                              kAuxiliaryContextAttribsCompat);

    if (!NAMESPACED_EGL(eglMakeCurrent)(dpy, surface, surface, context)) {
        fprintf(stderr, "%s: eglMakeCurrent failed\n", __func__);
        return false;
    }

    if (context_out) *context_out = context;
    if (surface_out) *surface_out = surface;

    return true;
}

static bool unbindAndDestroyAuxiliaryContext(EGLContext context, EGLSurface surface) {

    // create the context
    EGLDisplay dpy = NAMESPACED_EGL(eglGetDisplay)(EGL_DEFAULT_DISPLAY);

    if (!NAMESPACED_EGL(eglMakeCurrent)(
            dpy, EGL_NO_SURFACE, EGL_NO_SURFACE,
            EGL_NO_CONTEXT)) {
        fprintf(stderr, "%s: failure to unbind current context!\n",
                __func__);
        return false;
    }


    if (!eglDestroySurface(dpy, surface)) {
        fprintf(stderr, "%s: failure to destroy surface!\n",
                __func__);
        return false;
    }

    if (!eglDestroyContext(dpy, context)) {
        fprintf(stderr, "%s: failure to destroy context!\n",
                __func__);
        return false;
    }

    return true;
}

static bool bindAuxiliaryContext(EGLContext context, EGLSurface surface) {
    // create the context
    EGLDisplay dpy = NAMESPACED_EGL(eglGetDisplay)(EGL_DEFAULT_DISPLAY);

    if (!eglMakeCurrent(dpy, surface, surface, context)) {
        fprintf(stderr, "%s: eglMakeCurrent failed\n", __func__);
        return false;
    }

    return true;
}

static bool unbindAuxiliaryContext() {

    // create the context
    EGLDisplay dpy = NAMESPACED_EGL(eglGetDisplay)(EGL_DEFAULT_DISPLAY);

    if (!eglMakeCurrent(
            dpy, EGL_NO_SURFACE, EGL_NO_SURFACE,
            EGL_NO_CONTEXT)) {
        fprintf(stderr, "%s: failure to unbind current context!\n",
                __func__);
        return false;
    }

    return true;
}

EGLAPI EGLint EGLAPIENTRY eglGetError(void) {
    MEM_TRACE("EMUGL");
    CURRENT_THREAD();
    EGLint err = tls_thread->getError();
    tls_thread->setError(EGL_SUCCESS);
    return err;
}


} // namespace translator
} // namespace egl
