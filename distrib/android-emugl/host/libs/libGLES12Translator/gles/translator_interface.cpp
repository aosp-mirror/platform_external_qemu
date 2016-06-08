/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gles_context.h"
#include "egl_image.h"
#include "GLES/glext.h"

#include "translator_interface.h"
#include "gles/macros.h"

#include "android/base/threads/ThreadStore.h"

#include "GLES12Translator/underlying_apis.h"
#include "GLES12Translator/angle_gles2.h"

#include <stdio.h>

static GlesContext* the_context = NULL;

// Interface to emulator

class GLES12ThreadInfo {
public:
    GLES12ThreadInfo() : context(NULL) { }
    explicit GLES12ThreadInfo(GlesContext* _cxt) : context(_cxt) { }
    GlesContext* context;
};

typedef android::base::ThreadStore<GLES12ThreadInfo*> EmulatedGLES1ThreadInfo;

static EmulatedGLES1ThreadInfo sGLES12ThreadInfo;

GlesContext* CreateGles1Context(GlesContext* share,
                                const UnderlyingApis* underlying_apis) {
    return new GlesContext(0, kGles11, share, NULL, underlying_apis);
}

GlesContext* GetCurrentGlesContext() {
    return ((GLES12ThreadInfo*)sGLES12ThreadInfo.get())->context;
}

void SetCurrentGlesContext(GlesContext* cxt_in) {
    GLES12ThreadInfo* tInfo = (GLES12ThreadInfo*)sGLES12ThreadInfo.get();
    tInfo->context = cxt_in;
}

TRANSLATOR_APIENTRY(void*, create_underlying_api) {
    UnderlyingApis* container = new UnderlyingApis;
    container->angle = new ANGLE_GLES2;
    return (void*)container;
}

TRANSLATOR_APIENTRY(void*, create_gles1_context, void* share, const void* underlying_apis) {
    return (void*)CreateGles1Context((GlesContext*)share, (const UnderlyingApis*)underlying_apis);
}

TRANSLATOR_APIENTRY(void*, get_current_gles_context) {
    return (void*)((GLES12ThreadInfo*)sGLES12ThreadInfo.get())->context;
}

TRANSLATOR_APIENTRY(void, set_current_gles_context, void* cxt_in) {
    SetCurrentGlesContext((GlesContext*)cxt_in);
}

void* MapTexSubImage2DCHROMIUMCall (const GlesContext* c,
                                    GLenum target, GLint level,
                                    GLint xoffset, GLint yoffset,
                                    GLsizei width, GLsizei height,
                                    GLenum format, GLenum type, GLenum access)
{
    fprintf(stderr, "%s called", __FUNCTION__);
    return NULL;
}

void  UnmapTexSubImage2DCHROMIUMCall (const GlesContext* c, const void* mem)
{
    fprintf(stderr, "%s called", __FUNCTION__);
    return;
}

void* MapBufferSubDataCHROMIUMCall (GLuint target, GLintptr offset,
                                    GLsizeiptr size, GLenum access)
{
    fprintf(stderr, "%s called", __FUNCTION__);
    return NULL;
}

EglImagePtr GetEglImageFromNativeBuffer(GLeglImageOES buffer)
{
    GLenum target = 0;
    GLuint texture = 0;
    return EglImage::Create(target, texture);
}
