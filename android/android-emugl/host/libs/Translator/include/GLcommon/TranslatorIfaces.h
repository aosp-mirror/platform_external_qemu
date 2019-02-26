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
#ifndef TRANSLATOR_IFACES_H
#define TRANSLATOR_IFACES_H

#include "android/base/containers/SmallVector.h"
#include "GLcommon/GLutils.h"
#include "GLcommon/ShareGroup.h"

#include <EGL/egl.h>
#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>
#include <string.h>

#include <unordered_map>

extern "C" {

/* This is a generic function pointer type, whose name indicates it must
 * be cast to the proper type *and calling convention* before use.
 */
typedef void (*__translatorMustCastToProperFunctionPointerType)(void);

typedef struct {
  const char*                                     name;
  __translatorMustCastToProperFunctionPointerType address;
} ExtensionDescriptor;

class SaveableTexture;
typedef std::shared_ptr<SaveableTexture> SaveableTexturePtr;

struct EglImage
{
    ~EglImage(){};
    unsigned int imageId;
    NamedObjectPtr globalTexObj;
    unsigned int width;
    unsigned int height;
    unsigned int internalFormat;
    unsigned int border;
    unsigned int format;
    unsigned int type;
    unsigned int texStorageLevels;
    SaveableTexturePtr saveableTexture;
    bool needRestore;
    GLsync sync;
};

typedef emugl::SmartPtr<EglImage> ImagePtr;
typedef std::unordered_map<unsigned int, ImagePtr> ImagesHndlMap;
typedef std::unordered_map<unsigned int, SaveableTexturePtr> SaveableTextureMap;

class GLEScontext;
class SaveableTexture;

namespace android_studio {
    class EmulatorGLESUsages;
}

typedef struct {
    void (*initGLESx)(bool isGles2Gles);
    GLEScontext*                                    (*createGLESContext)(int majorVersion, int minorVersion, GlobalNameSpace* globalNameSpace, android::base::Stream* stream);
    void                                            (*initContext)(GLEScontext*,ShareGroupPtr);
    void                                            (*setMaxGlesVersion)(GLESVersion);
    void                                            (*deleteGLESContext)(GLEScontext*);
    void                                            (*flush)();
    void                                            (*finish)();
    int                                             (*getError)();
    void                                            (*setShareGroup)(GLEScontext*,ShareGroupPtr);
    __translatorMustCastToProperFunctionPointerType (*getProcAddress)(const char*);
    GLsync                                          (*fenceSync)(GLenum, GLbitfield);
    GLenum                                          (*clientWaitSync)(GLsync, GLbitfield, GLuint64);
    void                                            (*waitSync)(GLsync, GLbitfield, GLuint64);
    void                                            (*deleteSync)(GLsync);
    void                                            (*preSaveTexture)();
    void                                            (*postSaveTexture)();
    void                                            (*saveTexture)(SaveableTexture*, android::base::Stream*, android::base::SmallVector<unsigned char>* buffer);
    SaveableTexture* (*createTexture)(GlobalNameSpace*,
                                      std::function<void(SaveableTexture*)>&&);
    void                                            (*restoreTexture)(SaveableTexture*);
    void                                            (*deleteRbo)(GLuint);
    void                                            (*blitFromCurrentReadBufferANDROID)(EGLImage);
    void                                            (*fillGLESUsages)(android_studio::EmulatorGLESUsages*);
    bool                                            (*vulkanInteropSupported)();
} GLESiface;

class GlLibrary;

// A structure containing function pointers implemented by the EGL
// translator library, and called from the GLES 1.x and 2.0 translator
// libraries.
struct EGLiface {
    // Get the current GLESContext instance for the current thread.
    GLEScontext* (*getGLESContext)();

    // Retrieve a shared pointer to a global EglImage object named |imageId|
    ImagePtr    (*getEGLImage)(unsigned int imageId);

    // Return instance of GlLibrary class to use for dispatch.
    // at runtime. This is implemented in the EGL library because on Windows
    // all functions returned by wglGetProcAddress() are context-dependent!
    GlLibrary* (*eglGetGlLibrary)();

    // Context creation functions for auxiliary functions (e.g.,
    // background loading). Instead of going through the standard
    // eglChooseConfig/eglCreatePbufferSurface/eglCreateContext/eglMakeCurrent,
    // these functions set up a minimal context with pre-set configuration
    // that allows one to easily run GL calls in separate threads.
    // createAndBindAuxiliaryContext(): Creates a minimal context (1x1 pbuffer, with first config)
    // and makes it the current context and writes the resulting context and surface to
    // |context_out| and |surface_out|. Returns false if any step of context creation failed,
    // true otherwise.
    bool (*createAndBindAuxiliaryContext)(EGLContext* context_out, EGLSurface* surface_out);
    // unbindAndDestroyAuxiliaryContext(): Cleans up the resulting |context| and |surface| from a
    // createAndBindAuxiliaryContext() call; binds the null context and surface, then
    // destroys the surface and context. Returns false if any step of cleanup failed,
    // true otherwise.
    bool (*unbindAndDestroyAuxiliaryContext)(EGLContext context, EGLSurface surface);
    // Makes |context| and |surface| current, assuming they have been created already.
    bool (*bindAuxiliaryContext)(EGLContext context, EGLSurface surface);
    // Makes EGL_NO_SURFACE and EGL_NO_CONTEXT current, unbinding whatever is the current context.
    bool (*unbindAuxiliaryContext)();
};

typedef GLESiface* (*__translator_getGLESIfaceFunc)(const EGLiface*);

}
#endif
