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

#include "GLcommon/objectNameManager.h"

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

class TextureData : public ObjectData
{
public:
    TextureData():  ObjectData(TEXTURE_DATA),
                    width(0),
                    height(0),
                    depth(0),
                    border(0),
                    internalFormat(GL_RGBA),
                    sourceEGLImage(0),
                    wasBound(false),
                    requiresAutoMipmap(false),
                    target(0) {
        memset(crop_rect,0,4*sizeof(int));
    };

    unsigned int width;
    unsigned int height;
    unsigned int depth;
    unsigned int border;
    unsigned int internalFormat;
    unsigned int sourceEGLImage;
    bool wasBound;
    bool requiresAutoMipmap;
    int          crop_rect[4];
    GLenum target;
};

struct EglImage
{
    ~EglImage(){};
    unsigned int imageId;
    NamedObjectPtr globalTexObj;
    unsigned int width;
    unsigned int height;
    unsigned int internalFormat;
    unsigned int border;
};

typedef emugl::SmartPtr<EglImage> ImagePtr;
typedef std::unordered_map<unsigned int, ImagePtr> ImagesHndlMap;

class GLEScontext;

typedef struct {
    void                                            (*initGLESx)();
    GLEScontext*                                    (*createGLESContext)(int majorVersion, int minorVersion);
    void                                            (*initContext)(GLEScontext*,ShareGroupPtr);
    void                                            (*deleteGLESContext)(GLEScontext*);
    void                                            (*flush)();
    void                                            (*finish)();
    int                                             (*getError)();
    void                                            (*setShareGroup)(GLEScontext*,ShareGroupPtr);
    __translatorMustCastToProperFunctionPointerType (*getProcAddress)(const char*);
    GLsync                                          (*fenceSync)(GLenum, GLbitfield);
    GLenum                                          (*clientWaitSync)(GLsync, GLbitfield, GLuint64);
    void                                            (*deleteSync)(GLsync);
}GLESiface;

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
};

typedef GLESiface* (*__translator_getGLESIfaceFunc)(const EGLiface*);

}
#endif
