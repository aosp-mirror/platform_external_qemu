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
#undef GL_API
#define GL_API __declspec(dllexport)
#define GL_APICALL __declspec(dllexport)
#endif
#define GL_GLEXT_PROTOTYPES
#include "android/base/memory/LazyInstance.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "GLEScmContext.h"
#include "GLEScmValidate.h"
#include "GLEScmUtils.h"
#include <GLcommon/TextureUtils.h>
#include <OpenglCodecCommon/ErrorLog.h>

#include <GLcommon/GLDispatch.h>
#include <GLcommon/GLconversion_macros.h>
#include <GLcommon/TextureData.h>
#include <GLcommon/TranslatorIfaces.h>
#include <GLcommon/FramebufferData.h>

#include "emugl/common/crash_reporter.h"

#include <cmath>
#include <unordered_map>

#include <stdio.h>

#define DEBUG 0

#if DEBUG
#define GLES_CM_TRACE() fprintf(stderr, "%s\n", __func__); ERRCHECK()
#else
#define GLES_CM_TRACE()
#endif

extern "C" {

//decleration
static void initGLESx(bool isGles2Gles);
static void initContext(GLEScontext* ctx,ShareGroupPtr grp);
static void setMaxGlesVersion(GLESVersion version);
static void deleteGLESContext(GLEScontext* ctx);
static void setShareGroup(GLEScontext* ctx,ShareGroupPtr grp);
static GLEScontext* createGLESContext(int maj, int min,
        GlobalNameSpace* globalNameSpace, android::base::Stream* stream);
static __translatorMustCastToProperFunctionPointerType getProcAddress(const char* procName);
static void fillGLESUsages(android_studio::EmulatorGLESUsages* usage);
static bool vulkanInteropSupported();
}

/************************************** GLES EXTENSIONS *********************************************************/
typedef std::unordered_map<std::string, __translatorMustCastToProperFunctionPointerType> ProcTableMap;
ProcTableMap *s_glesExtensions = NULL;
/****************************************************************************************************************/

static EGLiface*  s_eglIface = NULL;
static GLESiface  s_glesIface = {
    .initGLESx                        = initGLESx,
    .createGLESContext                = createGLESContext,
    .initContext                      = initContext,
    .setMaxGlesVersion                = setMaxGlesVersion,
    .deleteGLESContext                = deleteGLESContext,
    .flush                            = (FUNCPTR_NO_ARGS_RET_VOID)glFlush,
    .finish                           = (FUNCPTR_NO_ARGS_RET_VOID)glFinish,
    .getError                         = (FUNCPTR_NO_ARGS_RET_INT)glGetError,
    .setShareGroup                    = setShareGroup,
    .getProcAddress                   = getProcAddress,
    .fenceSync                        = NULL,
    .clientWaitSync                   = NULL,
    .waitSync                         = NULL,
    .deleteSync                       = NULL,
    .preSaveTexture                   = NULL,
    .postSaveTexture                  = NULL,
    .saveTexture                      = NULL,
    .createTexture                    = NULL,
    .restoreTexture                   = NULL,
    .deleteRbo                        = NULL,
    .blitFromCurrentReadBufferANDROID = NULL,
    .fillGLESUsages = fillGLESUsages,
    .vulkanInteropSupported = vulkanInteropSupported,
};

#include <GLcommon/GLESmacros.h>

static android::base::LazyInstance<android_studio::EmulatorGLEScmUsages> gles1usages = {};

extern "C" {

static void setMaxGlesVersion(GLESVersion version) {
    GLEScmContext::setMaxGlesVersion(version);
}

#define ERRCHECK() { \
    GLint err = ctx->dispatcher().glGetError(); \
    if (err) fprintf(stderr, "%s:%d GL err 0x%x\n", __func__, __LINE__, err); } \

#define CORE_ERR_FORWARD() \
    if (isCoreProfile()) { \
        GLint __core_error = ctx->getErrorCoreProfile(); \
         SET_ERROR_IF(__core_error, __core_error); \
    } \

static void initGLESx(bool isGles2Gles) {
    setGles2Gles(isGles2Gles);
    return;
}

static void initContext(GLEScontext* ctx,ShareGroupPtr grp) {
    setCoreProfile(ctx->isCoreProfile());
    GLEScmContext::initGlobal(s_eglIface);

    // TODO: Properly restore GLES1 context from snapshot.
    // This is just to avoid a crash.
    if (ctx->needRestore()) {
        // TODO: metrics for GLES1 snapshots
        fprintf(stderr,
                "Warning: restoring GLES1 context from snapshot. App "
                "may need reloading.\n");
    }

    if (!ctx->shareGroup()) {
        ctx->setShareGroup(grp);
    }
    if (!ctx->isInitialized()) {
        ctx->init();
        glBindTexture(GL_TEXTURE_2D,0);
        glBindTexture(GL_TEXTURE_CUBE_MAP_OES,0);
    }
    if (ctx->needRestore()) {
        ctx->restore();
    }
}

static GLEScontext* createGLESContext(int maj, int min,
        GlobalNameSpace* globalNameSpace, android::base::Stream* stream) {
    (void)stream;
    return new GLEScmContext(maj, min, globalNameSpace, stream);
}

static void deleteGLESContext(GLEScontext* ctx) {
    if(ctx) delete ctx;
}

static void setShareGroup(GLEScontext* ctx,ShareGroupPtr grp) {
    if(ctx) {
        ctx->setShareGroup(grp);
    }
}

static void fillGLESUsages(android_studio::EmulatorGLESUsages* usage) {
    usage->mutable_gles_1_usages()->CopyFrom(*gles1usages);
}

static bool vulkanInteropSupported() {
    return GLEScontext::vulkanInteropSupported();
}

GL_API void GL_APIENTRY  glColorPointerWithDataSize( GLint size, GLenum type,
        GLsizei stride, const GLvoid *pointer, GLsizei dataSize);
GL_API void GL_APIENTRY  glNormalPointerWithDataSize(GLenum type,
        GLsizei stride, const GLvoid *pointer, GLsizei dataSize);
GL_API void GL_APIENTRY  glTexCoordPointerWithDataSize( GLint size, GLenum type,
        GLsizei stride, const GLvoid *pointer, GLsizei dataSize);
GL_API void GL_APIENTRY  glVertexPointerWithDataSize( GLint size, GLenum type,
        GLsizei stride, const GLvoid *pointer, GLsizei dataSize);

static __translatorMustCastToProperFunctionPointerType getProcAddress(const char* procName) {
    GET_CTX_RET(NULL)
    ctx->getGlobalLock();
    static bool proc_table_initialized = false;
    if (!proc_table_initialized) {
        proc_table_initialized = true;
        if (!s_glesExtensions)
            s_glesExtensions = new ProcTableMap();
        else
            s_glesExtensions->clear();
        (*s_glesExtensions)["glEGLImageTargetTexture2DOES"] = (__translatorMustCastToProperFunctionPointerType)glEGLImageTargetTexture2DOES;
        (*s_glesExtensions)["glEGLImageTargetRenderbufferStorageOES"]=(__translatorMustCastToProperFunctionPointerType)glEGLImageTargetRenderbufferStorageOES;
        (*s_glesExtensions)["glBlendEquationSeparateOES"] = (__translatorMustCastToProperFunctionPointerType)glBlendEquationSeparateOES;
        (*s_glesExtensions)["glBlendFuncSeparateOES"] = (__translatorMustCastToProperFunctionPointerType)glBlendFuncSeparateOES;
        (*s_glesExtensions)["glBlendEquationOES"] = (__translatorMustCastToProperFunctionPointerType)glBlendEquationOES;

        if (ctx->getCaps()->GL_ARB_MATRIX_PALETTE && ctx->getCaps()->GL_ARB_VERTEX_BLEND) {
            (*s_glesExtensions)["glCurrentPaletteMatrixOES"] = (__translatorMustCastToProperFunctionPointerType)glCurrentPaletteMatrixOES;
            (*s_glesExtensions)["glLoadPaletteFromModelViewMatrixOES"]=(__translatorMustCastToProperFunctionPointerType)glLoadPaletteFromModelViewMatrixOES;
            (*s_glesExtensions)["glMatrixIndexPointerOES"] = (__translatorMustCastToProperFunctionPointerType)glMatrixIndexPointerOES;
            (*s_glesExtensions)["glWeightPointerOES"] = (__translatorMustCastToProperFunctionPointerType)glWeightPointerOES;
        }
        (*s_glesExtensions)["glDepthRangefOES"] = (__translatorMustCastToProperFunctionPointerType)glDepthRangef;
        (*s_glesExtensions)["glFrustumfOES"] = (__translatorMustCastToProperFunctionPointerType)glFrustumf;
        (*s_glesExtensions)["glOrthofOES"] = (__translatorMustCastToProperFunctionPointerType)glOrthof;
        (*s_glesExtensions)["glClipPlanefOES"] = (__translatorMustCastToProperFunctionPointerType)glClipPlanef;
        (*s_glesExtensions)["glGetClipPlanefOES"] = (__translatorMustCastToProperFunctionPointerType)glGetClipPlanef;
        (*s_glesExtensions)["glClearDepthfOES"] = (__translatorMustCastToProperFunctionPointerType)glClearDepthf;
        (*s_glesExtensions)["glPointSizePointerOES"] = (__translatorMustCastToProperFunctionPointerType)glPointSizePointerOES;
        (*s_glesExtensions)["glTexGenfOES"] = (__translatorMustCastToProperFunctionPointerType)glTexGenfOES;
        (*s_glesExtensions)["glTexGenfvOES"] = (__translatorMustCastToProperFunctionPointerType)glTexGenfvOES;
        (*s_glesExtensions)["glTexGeniOES"] = (__translatorMustCastToProperFunctionPointerType)glTexGeniOES;
        (*s_glesExtensions)["glTexGenivOES"] = (__translatorMustCastToProperFunctionPointerType)glTexGenivOES;
        (*s_glesExtensions)["glTexGenxOES"] = (__translatorMustCastToProperFunctionPointerType)glTexGenxOES;
        (*s_glesExtensions)["glTexGenxvOES"] = (__translatorMustCastToProperFunctionPointerType)glTexGenxvOES;
        (*s_glesExtensions)["glGetTexGenfvOES"] = (__translatorMustCastToProperFunctionPointerType)glGetTexGenfvOES;
        (*s_glesExtensions)["glGetTexGenivOES"] = (__translatorMustCastToProperFunctionPointerType)glGetTexGenivOES;
        (*s_glesExtensions)["glGetTexGenxvOES"] = (__translatorMustCastToProperFunctionPointerType)glGetTexGenxvOES;
        if (ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT) {
            (*s_glesExtensions)["glIsRenderbufferOES"] = (__translatorMustCastToProperFunctionPointerType)glIsRenderbufferOES;
            (*s_glesExtensions)["glBindRenderbufferOES"] = (__translatorMustCastToProperFunctionPointerType)glBindRenderbufferOES;
            (*s_glesExtensions)["glDeleteRenderbuffersOES"] = (__translatorMustCastToProperFunctionPointerType)glDeleteRenderbuffersOES;
            (*s_glesExtensions)["glGenRenderbuffersOES"] = (__translatorMustCastToProperFunctionPointerType)glGenRenderbuffersOES;
            (*s_glesExtensions)["glRenderbufferStorageOES"] = (__translatorMustCastToProperFunctionPointerType)glRenderbufferStorageOES;
            (*s_glesExtensions)["glGetRenderbufferParameterivOES"] = (__translatorMustCastToProperFunctionPointerType)glGetRenderbufferParameterivOES;
            (*s_glesExtensions)["glIsFramebufferOES"] = (__translatorMustCastToProperFunctionPointerType)glIsFramebufferOES;
            (*s_glesExtensions)["glBindFramebufferOES"] = (__translatorMustCastToProperFunctionPointerType)glBindFramebufferOES;
            (*s_glesExtensions)["glDeleteFramebuffersOES"] = (__translatorMustCastToProperFunctionPointerType)glDeleteFramebuffersOES;
            (*s_glesExtensions)["glGenFramebuffersOES"] = (__translatorMustCastToProperFunctionPointerType)glGenFramebuffersOES;
            (*s_glesExtensions)["glCheckFramebufferStatusOES"] = (__translatorMustCastToProperFunctionPointerType)glCheckFramebufferStatusOES;
            (*s_glesExtensions)["glFramebufferTexture2DOES"] = (__translatorMustCastToProperFunctionPointerType)glFramebufferTexture2DOES;
            (*s_glesExtensions)["glFramebufferRenderbufferOES"] = (__translatorMustCastToProperFunctionPointerType)glFramebufferRenderbufferOES;
            (*s_glesExtensions)["glGetFramebufferAttachmentParameterivOES"] = (__translatorMustCastToProperFunctionPointerType)glGetFramebufferAttachmentParameterivOES;
            (*s_glesExtensions)["glGenerateMipmapOES"] = (__translatorMustCastToProperFunctionPointerType)glGenerateMipmapOES;
        }
        (*s_glesExtensions)["glDrawTexsOES"] = (__translatorMustCastToProperFunctionPointerType)glDrawTexsOES;
        (*s_glesExtensions)["glDrawTexiOES"] = (__translatorMustCastToProperFunctionPointerType)glDrawTexiOES;
        (*s_glesExtensions)["glDrawTexfOES"] = (__translatorMustCastToProperFunctionPointerType)glDrawTexfOES;
        (*s_glesExtensions)["glDrawTexxOES"] = (__translatorMustCastToProperFunctionPointerType)glDrawTexxOES;
        (*s_glesExtensions)["glDrawTexsvOES"] = (__translatorMustCastToProperFunctionPointerType)glDrawTexsvOES;
        (*s_glesExtensions)["glDrawTexivOES"] = (__translatorMustCastToProperFunctionPointerType)glDrawTexivOES;
        (*s_glesExtensions)["glDrawTexfvOES"] = (__translatorMustCastToProperFunctionPointerType)glDrawTexfvOES;
        (*s_glesExtensions)["glDrawTexxvOES"] = (__translatorMustCastToProperFunctionPointerType)glDrawTexxvOES;

        // Vertex array functions with size for snapshot
        (*s_glesExtensions)["glColorPointerWithDataSize"] = (__translatorMustCastToProperFunctionPointerType)glColorPointerWithDataSize;
        (*s_glesExtensions)["glNormalPointerWithDataSize"] = (__translatorMustCastToProperFunctionPointerType)glNormalPointerWithDataSize;
        (*s_glesExtensions)["glTexCoordPointerWithDataSize"] = (__translatorMustCastToProperFunctionPointerType)glTexCoordPointerWithDataSize;
        (*s_glesExtensions)["glVertexPointerWithDataSize"] = (__translatorMustCastToProperFunctionPointerType)glVertexPointerWithDataSize;
    }
    __translatorMustCastToProperFunctionPointerType ret=NULL;
    ProcTableMap::iterator val = s_glesExtensions->find(procName);
    if (val!=s_glesExtensions->end())
        ret = val->second;
    ctx->releaseGlobalLock();

    return ret;
}

GL_APICALL GLESiface* GL_APIENTRY __translator_getIfaces(EGLiface* eglIface);

GLESiface* __translator_getIfaces(EGLiface* eglIface) {
    s_eglIface = eglIface;
    return &s_glesIface;
}

} // extern "C"

static TextureData* getTextureData(ObjectLocalName tex){
    GET_CTX_RET(NULL);

    if (!ctx->shareGroup()->isObject(NamedObjectType::TEXTURE, tex)) {
        return NULL;
    }

    TextureData *texData = NULL;
    auto objData =
            ctx->shareGroup()->getObjectData(NamedObjectType::TEXTURE, tex);
    if(!objData){
        texData = new TextureData();
        ctx->shareGroup()->setObjectData(NamedObjectType::TEXTURE, tex,
                                         ObjectDataPtr(texData));
    } else {
        texData = (TextureData*)objData;
    }
    return texData;
}

static TextureData* getTextureTargetData(GLenum target){
    GET_CTX_RET(NULL);
    unsigned int tex = ctx->getBindedTexture(target);
    return getTextureData(ctx->getTextureLocalName(target,tex));
}

GL_API GLboolean GL_APIENTRY glIsBuffer(GLuint buffer) {
    GET_CTX_RET(GL_FALSE)
    GLES_CM_TRACE()

    if(buffer && ctx->shareGroup().get()) {
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::VERTEXBUFFER, buffer);
        return objData ? ((GLESbuffer*)objData)->wasBinded()
                             : GL_FALSE;
    }
    return GL_FALSE;
}

GL_API GLboolean GL_APIENTRY  glIsEnabled( GLenum cap) {
    GET_CTX_CM_RET(GL_FALSE)
    GLES_CM_TRACE()
    RET_AND_SET_ERROR_IF(!GLEScmValidate::capability(cap,ctx->getMaxLights(),ctx->getMaxClipPlanes()),GL_INVALID_ENUM,GL_FALSE);

    if (cap == GL_POINT_SIZE_ARRAY_OES)
        return ctx->isArrEnabled(cap);
    else if (cap==GL_TEXTURE_GEN_STR_OES)
        return (ctx->dispatcher().glIsEnabled(GL_TEXTURE_GEN_S) &&
                ctx->dispatcher().glIsEnabled(GL_TEXTURE_GEN_T) &&
                ctx->dispatcher().glIsEnabled(GL_TEXTURE_GEN_R));
    else
        return ctx->dispatcher().glIsEnabled(cap);
}

GL_API GLboolean GL_APIENTRY  glIsTexture( GLuint texture) {
    GET_CTX_RET(GL_FALSE)
    GLES_CM_TRACE()

    if(texture == 0) // Special case
        return GL_FALSE;

    TextureData* tex = getTextureData(texture);
    return tex ? tex->wasBound : GL_FALSE;
}

GL_API GLenum GL_APIENTRY  glGetError(void) {
    GET_CTX_RET(GL_NO_ERROR)
    GLES_CM_TRACE()
    GLenum err = ctx->getGLerror();
    if(err != GL_NO_ERROR) {
        ctx->setGLerror(GL_NO_ERROR);
        return err;
    }

    return ctx->dispatcher().glGetError();
}

GL_API const GLubyte * GL_APIENTRY  glGetString( GLenum name) {
    GET_CTX_RET(NULL)
    GLES_CM_TRACE()
    switch(name) {
        case GL_VENDOR:
            return (const GLubyte*)ctx->getVendorString();
        case GL_RENDERER:
            return (const GLubyte*)ctx->getRendererString();
        case GL_VERSION:
            return (const GLubyte*)ctx->getVersionString();
        case GL_EXTENSIONS:
            return (const GLubyte*)ctx->getExtensionString();
        default:
            RET_AND_SET_ERROR_IF(true,GL_INVALID_ENUM,NULL);
    }
}

GL_API void GL_APIENTRY  glActiveTexture( GLenum texture) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::textureEnum(texture,ctx->getMaxTexUnits()),GL_INVALID_ENUM);
    ctx->setActiveTexture(texture);
    ctx->dispatcher().glActiveTexture(texture);
}

GL_API void GL_APIENTRY  glAlphaFunc( GLenum func, GLclampf ref) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::alphaFunc(func),GL_INVALID_ENUM);
    ctx->dispatcher().glAlphaFunc(func,ref);
}


GL_API void GL_APIENTRY  glAlphaFuncx( GLenum func, GLclampx ref) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::alphaFunc(func),GL_INVALID_ENUM);
    ctx->dispatcher().glAlphaFunc(func,X2F(ref));
}


GL_API void GL_APIENTRY  glBindBuffer( GLenum target, GLuint buffer) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::bufferTarget(target),GL_INVALID_ENUM);

    //if buffer wasn't generated before,generate one
    if (buffer && ctx->shareGroup().get() &&
        !ctx->shareGroup()->isObject(NamedObjectType::VERTEXBUFFER, buffer)) {
        ctx->shareGroup()->genName(NamedObjectType::VERTEXBUFFER, buffer);
        ctx->shareGroup()->setObjectData(NamedObjectType::VERTEXBUFFER, buffer,
                                         ObjectDataPtr(new GLESbuffer()));
    }
    ctx->bindBuffer(target,buffer);
    ctx->dispatcher().glBindBuffer(target, ctx->shareGroup()->getGlobalName(
                NamedObjectType::VERTEXBUFFER, buffer));
    if (buffer) {
        GLESbuffer* vbo =
                (GLESbuffer*)ctx->shareGroup()
                        ->getObjectData(NamedObjectType::VERTEXBUFFER, buffer);
        vbo->setBinded();
    }
}


GL_API void GL_APIENTRY  glBindTexture( GLenum target, GLuint texture) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::textureTarget(target),GL_INVALID_ENUM)

    //for handling default texture (0)
    ObjectLocalName localTexName = ctx->getTextureLocalName(target,texture);

    GLuint globalTextureName = localTexName;
    if(ctx->shareGroup().get()){
        globalTextureName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::TEXTURE, localTexName);
        //if texture wasn't generated before,generate one
        if(!globalTextureName){
            ctx->shareGroup()->genName(NamedObjectType::TEXTURE, localTexName);
            globalTextureName = ctx->shareGroup()->getGlobalName(
                    NamedObjectType::TEXTURE, localTexName);
        }

        TextureData* texData = getTextureData(localTexName);
        if (texData->target==0)
            texData->setTarget(target);
        //if texture was already bound to another target
        SET_ERROR_IF(ctx->GLTextureTargetToLocal(texData->target) != ctx->GLTextureTargetToLocal(target), GL_INVALID_OPERATION);
        texData->setGlobalName(globalTextureName);
        if (!texData->wasBound) {
            texData->resetSaveableTexture();
        }
        texData->wasBound = true;
    }

    ctx->setBindedTexture(target, texture, globalTextureName);
    ctx->dispatcher().glBindTexture(target, globalTextureName);
}

GL_API void GL_APIENTRY  glBlendFunc( GLenum sfactor, GLenum dfactor) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::blendSrc(sfactor) || !GLEScmValidate::blendDst(dfactor),GL_INVALID_ENUM)
    ctx->setBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
    ctx->dispatcher().glBlendFunc(sfactor,dfactor);
}

GL_API void GL_APIENTRY  glBufferData( GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::bufferTarget(target),GL_INVALID_ENUM);
    SET_ERROR_IF(!ctx->isBindedBuffer(target),GL_INVALID_OPERATION);
    ctx->setBufferData(target,size,data,usage);
    ctx->dispatcher().glBufferData(target, size, data, usage);
}

GL_API void GL_APIENTRY  glBufferSubData( GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!ctx->isBindedBuffer(target),GL_INVALID_OPERATION);
    SET_ERROR_IF(!GLEScmValidate::bufferTarget(target),GL_INVALID_ENUM);
    SET_ERROR_IF(!ctx->setBufferSubData(target,offset,size,data),GL_INVALID_VALUE);
    ctx->dispatcher().glBufferSubData(target, offset, size, data);
}

GL_API void GL_APIENTRY  glClear( GLbitfield mask) {
    GET_CTX()
    GLES_CM_TRACE()
    ERRCHECK()
    ctx->drawValidate();
    ERRCHECK()
    ctx->dispatcher().glClear(mask);
    ERRCHECK()
}

GL_API void GL_APIENTRY  glClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setClearColor(red, green, blue, alpha);
    ctx->dispatcher().glClearColor(red,green,blue,alpha);
}

GL_API void GL_APIENTRY  glClearColorx( GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setClearColor(X2F(red),X2F(green),X2F(blue),X2F(alpha));
    ctx->dispatcher().glClearColor(X2F(red),X2F(green),X2F(blue),X2F(alpha));
}


GL_API void GL_APIENTRY  glClearDepthf( GLclampf depth) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setClearDepth(depth);
    ctx->dispatcher().glClearDepth(depth);
}

GL_API void GL_APIENTRY  glClearDepthx( GLclampx depth) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setClearDepth(X2F(depth));
    ctx->dispatcher().glClearDepth(X2F(depth));
}

GL_API void GL_APIENTRY  glClearStencil( GLint s) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setClearStencil(s);
    ctx->dispatcher().glClearStencil(s);
}

GL_API void GL_APIENTRY  glClientActiveTexture( GLenum texture) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::textureEnum(texture,ctx->getMaxTexUnits()),GL_INVALID_ENUM);
    ctx->clientActiveTexture(texture);
}

GL_API void GL_APIENTRY  glClipPlanef( GLenum plane, const GLfloat *equation) {
    GET_CTX()
    GLES_CM_TRACE()
    GLdouble tmpEquation[4];

    for(int i = 0; i < 4; i++) {
         tmpEquation[i] = static_cast<GLdouble>(equation[i]);
    }
    ctx->dispatcher().glClipPlane(plane,tmpEquation);
}

GL_API void GL_APIENTRY  glClipPlanex( GLenum plane, const GLfixed *equation) {
    GET_CTX()
    GLES_CM_TRACE()
    GLdouble tmpEquation[4];
    for(int i = 0; i < 4; i++) {
        tmpEquation[i] = X2D(equation[i]);
    }
    ctx->dispatcher().glClipPlane(plane,tmpEquation);
}

GL_API void GL_APIENTRY  glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->color4f(red, green, blue, alpha);
}

GL_API void GL_APIENTRY  glColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->color4ub(red, green, blue, alpha);
}

GL_API void GL_APIENTRY  glColor4x( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->color4f(X2F(red),X2F(green),X2F(blue),X2F(alpha));
}

GL_API void GL_APIENTRY  glColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setColorMask(red, green, blue, alpha);
    ctx->dispatcher().glColorMask(red,green,blue,alpha);
}

GL_API void GL_APIENTRY  glColorPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::colorPointerParams(size,stride),GL_INVALID_VALUE);
    SET_ERROR_IF(!GLEScmValidate::colorPointerType(type),GL_INVALID_ENUM);
    ctx->setPointer(GL_COLOR_ARRAY,size,type,stride,pointer, 0);
}

GL_API void GL_APIENTRY  glColorPointerWithDataSize( GLint size, GLenum type,
        GLsizei stride, const GLvoid *pointer, GLsizei dataSize) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::colorPointerParams(size,stride),GL_INVALID_VALUE);
    SET_ERROR_IF(!GLEScmValidate::colorPointerType(type),GL_INVALID_ENUM);
    ctx->setPointer(GL_COLOR_ARRAY,size,type,stride,pointer, dataSize);
}

static int maxMipmapLevel(GLsizei width, GLsizei height) {
    // + 0.5 for potential floating point rounding issue
    return log2(std::max(width, height) + 0.5);
}

void s_glInitTexImage2D(GLenum target, GLint level, GLint internalformat,
        GLsizei width, GLsizei height, GLint border, GLenum* format,
        GLenum* type, GLint* internalformat_out,
        bool* needAutoMipmap) {
    GET_CTX();

    if (ctx->shareGroup().get()) {
        TextureData *texData = getTextureTargetData(target);

        if (texData) {
            texData->hasStorage = true;
            if (needAutoMipmap) {
                *needAutoMipmap = texData->requiresAutoMipmap;
            }
            if (texData->requiresAutoMipmap) {
                texData->setMipmapLevelAtLeast(maxMipmapLevel(width, height));
            } else {
                texData->setMipmapLevelAtLeast(
                        static_cast<unsigned int>(level));
            }
        }

        if (texData && level == 0) {
            assert(texData->target == GL_TEXTURE_2D ||
                    texData->target == GL_TEXTURE_CUBE_MAP);
            texData->internalFormat = internalformat;
            if (internalformat_out) {
                *internalformat_out = texData->internalFormat;
            }
            texData->width = width;
            texData->height = height;
            texData->border = border;
            if (format) texData->format = *format;
            if (type) texData->type = *type;

            if (texData->sourceEGLImage != 0) {
                //
                // This texture was a target of EGLImage,
                // but now it is re-defined so we need to
                // re-generate global texture name for it.
                //
                unsigned int tex = ctx->getBindedTexture(target);
                ctx->shareGroup()->genName(NamedObjectType::TEXTURE, tex,
                        false);
                unsigned int globalTextureName =
                    ctx->shareGroup()->getGlobalName(
                            NamedObjectType::TEXTURE, tex);
                ctx->dispatcher().glBindTexture(GL_TEXTURE_2D,
                        globalTextureName);
                texData->sourceEGLImage = 0;
                texData->setGlobalName(globalTextureName);
            }
            texData->resetSaveableTexture();
        }
        texData->makeDirty();
    }
}

GL_API void GL_APIENTRY  glCompressedTexImage2D( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::textureTargetEx(target),GL_INVALID_ENUM);
    SET_ERROR_IF(!data,GL_INVALID_OPERATION);

    doCompressedTexImage2D(ctx, target, level, internalformat,
                                width, height, border,
                                imageSize, data, glTexImage2D);
    TextureData* texData = getTextureTargetData(target);
    if (texData) {
        texData->compressed = true;
        texData->compressedFormat = internalformat;
    }
}

GL_API void GL_APIENTRY  glCompressedTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!(GLEScmValidate::texCompImgFrmt(format) && GLEScmValidate::textureTargetEx(target)),GL_INVALID_ENUM);
    SET_ERROR_IF(level < 0 || level > log2(ctx->getMaxTexSize()),GL_INVALID_VALUE)
    SET_ERROR_IF(!data,GL_INVALID_OPERATION);

    GLenum uncompressedFrmt;
    unsigned char* uncompressed = uncompressTexture(format,uncompressedFrmt,width,height,imageSize,data,level);
    ctx->dispatcher().glTexSubImage2D(target,level,xoffset,yoffset,width,height,uncompressedFrmt,GL_UNSIGNED_BYTE,uncompressed);
    delete uncompressed;
    TextureData* texData = getTextureTargetData(target);
    if (texData) {
        texData->setMipmapLevelAtLeast(level);
        texData->makeDirty();
    }
}

GL_API void GL_APIENTRY  glCopyTexImage2D( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!(GLEScmValidate::pixelFrmt(ctx,internalformat)
            && GLEScmValidate::textureTargetEx(target)),GL_INVALID_ENUM);
    SET_ERROR_IF(border != 0,GL_INVALID_VALUE);

    GLenum format = baseFormatOfInternalFormat((GLint)internalformat);
    GLenum type = accurateTypeOfInternalFormat((GLint)internalformat);
    s_glInitTexImage2D(
        target, level, internalformat, width, height, border,
        &format, &type, (GLint*)&internalformat, nullptr);

    TextureData* texData = getTextureTargetData(target);
    if (texData && isCoreProfile() &&
        isCoreProfileEmulatedFormat(texData->format)) {
        GLEScontext::prepareCoreProfileEmulatedTexture(
            getTextureTargetData(target),
            false, target, format, type,
            (GLint*)&internalformat, &format);
        ctx->copyTexImageWithEmulation(
            texData, false, target, level, internalformat,
            0, 0, x, y, width, height, border);
    } else {
        ctx->dispatcher().glCopyTexImage2D(
            target, level, internalformat,
            x, y, width, height, border);
    }
}

GL_API void GL_APIENTRY  glCopyTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::textureTargetEx(target),GL_INVALID_ENUM);
    if (ctx->shareGroup().get()){
        TextureData *texData = getTextureTargetData(target);
        SET_ERROR_IF(!texData, GL_INVALID_OPERATION);
        texData->makeDirty();
    }
    ctx->dispatcher().glCopyTexSubImage2D(target,level,xoffset,yoffset,x,y,width,height);
}

GL_API void GL_APIENTRY  glCullFace( GLenum mode) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setCullFace(mode);
    ctx->dispatcher().glCullFace(mode);
}

GL_API void GL_APIENTRY  glDeleteBuffers( GLsizei n, const GLuint *buffers) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(ctx->shareGroup().get()) {
        for(int i=0; i < n; i++){
            ctx->shareGroup()->deleteName(NamedObjectType::VERTEXBUFFER,
                                          buffers[i]);
            ctx->unbindBuffer(buffers[i]);
        }
    }
}

GL_API void GL_APIENTRY  glDeleteTextures( GLsizei n, const GLuint *textures) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(ctx->shareGroup().get()) {
        for(int i=0; i < n; i++){
            if(textures[i] != 0)
            {
                if(ctx->getBindedTexture(GL_TEXTURE_2D) == textures[i])
                    ctx->setBindedTexture(GL_TEXTURE_2D,0);
                if (ctx->getBindedTexture(GL_TEXTURE_CUBE_MAP) == textures[i])
                    ctx->setBindedTexture(GL_TEXTURE_CUBE_MAP,0);
                ctx->shareGroup()->deleteName(NamedObjectType::TEXTURE,
                                              textures[i]);
            }
        }
    }
}

GL_API void GL_APIENTRY  glDepthFunc( GLenum func) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setDepthFunc(func);
    ctx->dispatcher().glDepthFunc(func);
}

GL_API void GL_APIENTRY  glDepthMask( GLboolean flag) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setDepthMask(flag);
    ctx->dispatcher().glDepthMask(flag);
}

GL_API void GL_APIENTRY  glDepthRangef( GLclampf zNear, GLclampf zFar) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setDepthRangef(zNear, zFar);
    ctx->dispatcher().glDepthRange(zNear,zFar);
}

GL_API void GL_APIENTRY  glDepthRangex( GLclampx zNear, GLclampx zFar) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setDepthRangef(X2F(zNear),X2F(zFar));
    ctx->dispatcher().glDepthRange(X2F(zNear),X2F(zFar));
}

GL_API void GL_APIENTRY  glDisable( GLenum cap) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->disable(cap);
}

GL_API void GL_APIENTRY  glDisableClientState( GLenum array) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::supportedArrays(array),GL_INVALID_ENUM)

    ctx->enableArr(array,false);

    if(array != GL_POINT_SIZE_ARRAY_OES) ctx->disableClientState(array);
}


GL_API void GL_APIENTRY  glDrawArrays( GLenum mode, GLint first, GLsizei count) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(count < 0,GL_INVALID_VALUE)
    SET_ERROR_IF(!GLEScmValidate::drawMode(mode),GL_INVALID_ENUM)

    ctx->drawArrays(mode, first, count);
}

GL_API void GL_APIENTRY  glDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *elementsIndices) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(count < 0,GL_INVALID_VALUE)
    SET_ERROR_IF((!GLEScmValidate::drawMode(mode) || !GLEScmValidate::drawType(type)),GL_INVALID_ENUM)

    ctx->drawElements(mode, count, type, elementsIndices);
}

GL_API void GL_APIENTRY  glEnable( GLenum cap) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->enable(cap);
}

GL_API void GL_APIENTRY  glEnableClientState( GLenum array) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::supportedArrays(array),GL_INVALID_ENUM)

    ctx->enableArr(array,true);
    if(array != GL_POINT_SIZE_ARRAY_OES) {
        ctx->enableClientState(array);
    }
}

GL_API void GL_APIENTRY  glFinish( void) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->dispatcher().glFinish();
}

GL_API void GL_APIENTRY  glFlush( void) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->dispatcher().glFlush();
}

GL_API void GL_APIENTRY  glFogf( GLenum pname, GLfloat param) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->fogf(pname,param);
}

GL_API void GL_APIENTRY  glFogfv( GLenum pname, const GLfloat *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->fogfv(pname,params);
}

GL_API void GL_APIENTRY  glFogx( GLenum pname, GLfixed param) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->fogf(pname,(pname == GL_FOG_MODE)? static_cast<GLfloat>(param):X2F(param));
}

GL_API void GL_APIENTRY  glFogxv( GLenum pname, const GLfixed *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    if(pname == GL_FOG_MODE) {
        GLfloat tmpParam = static_cast<GLfloat>(params[0]);
        ctx->fogfv(pname,&tmpParam);
    } else {
        GLfloat tmpParams[4];
        for(int i=0; i< 4; i++) {
            tmpParams[i] = X2F(params[i]);
        }
        ctx->fogfv(pname,tmpParams);
    }

}

GL_API void GL_APIENTRY  glFrontFace( GLenum mode) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setFrontFace(mode);
    ctx->dispatcher().glFrontFace(mode);
}

GL_API void GL_APIENTRY  glFrustumf( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->frustumf(left, right, bottom, top, zNear,zFar);
    ERRCHECK()
}

GL_API void GL_APIENTRY  glFrustumx( GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->frustumf(X2F(left),X2F(right),X2F(bottom),X2F(top),X2F(zNear),X2F(zFar));
}

GL_API void GL_APIENTRY  glGenBuffers( GLsizei n, GLuint *buffers) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(ctx->shareGroup().get()) {
        for(int i=0; i<n ;i++) {
            buffers[i] = ctx->shareGroup()->genName(
                    NamedObjectType::VERTEXBUFFER, 0, true);
            //generating vbo object related to this buffer name
            ctx->shareGroup()->setObjectData(NamedObjectType::VERTEXBUFFER,
                                             buffers[i],
                                             ObjectDataPtr(new GLESbuffer()));
        }
    }
}

GL_API void GL_APIENTRY  glGenTextures( GLsizei n, GLuint *textures) {
    GET_CTX();
    GLES_CM_TRACE()
    if(ctx->shareGroup().get()) {
        for(int i=0; i<n ;i++) {
            textures[i] = ctx->shareGroup()->genName(NamedObjectType::TEXTURE,
                                                     0, true);
        }
    }
}

GL_API void GL_APIENTRY  glGetBooleanv( GLenum pname, GLboolean *params) {
    GET_CTX()
    GLES_CM_TRACE()

#define TO_GLBOOL(params, x) \
    *params = x ? GL_TRUE : GL_FALSE; \

    if(ctx->glGetBooleanv(pname, params))
    {
        return;
    }

    switch(pname)
    {
    case GL_FRAMEBUFFER_BINDING_OES:
    case GL_RENDERBUFFER_BINDING_OES:
        {
            GLint name;
            glGetIntegerv(pname,&name);
            *params = name!=0 ? GL_TRUE: GL_FALSE;
        }
        break;
    case GL_TEXTURE_GEN_STR_OES:
        {
            GLboolean state_s = GL_FALSE;
            GLboolean state_t = GL_FALSE;
            GLboolean state_r = GL_FALSE;
            ctx->dispatcher().glGetBooleanv(GL_TEXTURE_GEN_S,&state_s);
            ctx->dispatcher().glGetBooleanv(GL_TEXTURE_GEN_T,&state_t);
            ctx->dispatcher().glGetBooleanv(GL_TEXTURE_GEN_R,&state_r);
            *params = state_s && state_t && state_r ? GL_TRUE: GL_FALSE;
        }
        break;
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
        *params = (GLboolean)getCompressedFormats(NULL);
        break;
    case GL_COMPRESSED_TEXTURE_FORMATS:
        {
            int nparams = getCompressedFormats(NULL);
            if (nparams>0) {
                int * iparams = new int[nparams];
                getCompressedFormats(iparams);
                for (int i=0; i<nparams; i++) params[i] = (GLboolean)iparams[i];
                delete [] iparams;
            }
        }
        break;
    case GL_GENERATE_MIPMAP_HINT:
        if (isCoreProfile()) {
            TO_GLBOOL(params, ctx->getHint(GL_GENERATE_MIPMAP_HINT));
        } else {
            ctx->dispatcher().glGetBooleanv(pname,params);
        }
        break;
    case GL_RED_BITS:
    case GL_GREEN_BITS:
    case GL_BLUE_BITS:
    case GL_ALPHA_BITS:
    case GL_DEPTH_BITS:
    case GL_STENCIL_BITS:
        if (isCoreProfile()) {
            GLuint fboBinding = ctx->getFramebufferBinding(GL_DRAW_FRAMEBUFFER);
            TO_GLBOOL(params, ctx->queryCurrFboBits(fboBinding, pname));
        } else {
            ctx->dispatcher().glGetBooleanv(pname,params);
        }
        break;
    default:
        ctx->dispatcher().glGetBooleanv(pname,params);
    }
}

GL_API void GL_APIENTRY  glGetBufferParameteriv( GLenum target, GLenum pname, GLint *params) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!(GLEScmValidate::bufferTarget(target) && GLEScmValidate::bufferParam(pname)),GL_INVALID_ENUM);
    SET_ERROR_IF(!ctx->isBindedBuffer(target),GL_INVALID_OPERATION);
    switch(pname) {
    case GL_BUFFER_SIZE:
        ctx->getBufferSize(target,params);
        break;
    case GL_BUFFER_USAGE:
        ctx->getBufferUsage(target,params);
        break;
    }

}

GL_API void GL_APIENTRY  glGetClipPlanef( GLenum pname, GLfloat eqn[4]) {
    GET_CTX()
    GLES_CM_TRACE()
    GLdouble tmpEqn[4];

    ctx->dispatcher().glGetClipPlane(pname,tmpEqn);
    for(int i =0 ;i < 4; i++){
        eqn[i] = static_cast<GLfloat>(tmpEqn[i]);
    }
}

GL_API void GL_APIENTRY  glGetClipPlanex( GLenum pname, GLfixed eqn[4]) {
    GET_CTX()
    GLES_CM_TRACE()
    GLdouble tmpEqn[4];

    ctx->dispatcher().glGetClipPlane(pname,tmpEqn);
    for(int i =0 ;i < 4; i++){
        eqn[i] = F2X(tmpEqn[i]);
    }
}

GL_API void GL_APIENTRY  glGetFixedv( GLenum pname, GLfixed *params) {
    GET_CTX()
    GLES_CM_TRACE()

    if(ctx->glGetFixedv(pname, params))
    {
        return;
    }

    size_t nParams = glParamSize(pname);
    GLfloat fParams[16];

    switch(pname)
    {
    case GL_FRAMEBUFFER_BINDING_OES:
    case GL_RENDERBUFFER_BINDING_OES:
    case GL_TEXTURE_GEN_STR_OES:
        glGetFloatv(pname,&fParams[0]);
        break;
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
        *params = I2X(getCompressedFormats(NULL));
        return;
        break;
    case GL_COMPRESSED_TEXTURE_FORMATS:
        {
            int nparams = getCompressedFormats(NULL);
            if (nparams>0) {
                int * iparams = new int[nparams];
                getCompressedFormats(iparams);
                for (int i=0; i<nparams; i++) params[i] = I2X(iparams[i]);
                delete [] iparams;
            }
            return;
        }
        break;
    default:
        ctx->dispatcher().glGetFloatv(pname,fParams);
    }

    if (nParams)
    {
        for(size_t i =0 ; i < nParams;i++) {
            params[i] = F2X(fParams[i]);
        }
    }
}

GL_API void GL_APIENTRY  glGetFloatv( GLenum pname, GLfloat *params) {
    GET_CTX()
    GLES_CM_TRACE()

    if(ctx->glGetFloatv(pname, params))
    {
        return;
    }

    GLint i;

    switch (pname) {
    case GL_FRAMEBUFFER_BINDING_OES:
    case GL_RENDERBUFFER_BINDING_OES:
    case GL_TEXTURE_GEN_STR_OES:
        glGetIntegerv(pname,&i);
        *params = (GLfloat)i;
        break;
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
        *params = (GLfloat)getCompressedFormats(NULL);
        break;
    case GL_COMPRESSED_TEXTURE_FORMATS:
        {
            int nparams = getCompressedFormats(NULL);
            if (nparams>0) {
                int * iparams = new int[nparams];
                getCompressedFormats(iparams);
                for (int i=0; i<nparams; i++) params[i] = (GLfloat)iparams[i];
                delete [] iparams;
            }
        }
        break;
    default:
        ctx->dispatcher().glGetFloatv(pname,params);
    }
}

GL_API void GL_APIENTRY  glGetIntegerv( GLenum pname, GLint *params) {
    GET_CTX()
    GLES_CM_TRACE()

    if(ctx->glGetIntegerv(pname, params))
    {
        return;
    }

    GLint i;
    GLfloat f;

    switch(pname)
    {
    case GL_READ_BUFFER:
    case GL_DRAW_BUFFER0:
        if (ctx->shareGroup().get()) {
            ctx->dispatcher().glGetIntegerv(pname, &i);
            GLenum target = pname == GL_READ_BUFFER ? GL_READ_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER;
            if (ctx->isDefaultFBOBound(target) && (GLint)i == GL_COLOR_ATTACHMENT0) {
                i = GL_BACK;
            }
            *params = i;
        }
        break;
    case GL_TEXTURE_GEN_STR_OES:
        ctx->dispatcher().glGetIntegerv(GL_TEXTURE_GEN_S,&params[0]);
        break;
    case GL_FRAMEBUFFER_BINDING_OES:
        ctx->dispatcher().glGetIntegerv(pname,&i);
        *params = ctx->getFBOLocalName(i);
        break;
    case GL_RENDERBUFFER_BINDING_OES:
        if (ctx->shareGroup().get()) {
            ctx->dispatcher().glGetIntegerv(pname,&i);
            *params = ctx->shareGroup()->getLocalName(
                    NamedObjectType::RENDERBUFFER, i);
        }
        break;
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
        *params = getCompressedFormats(NULL);
        break;
    case GL_COMPRESSED_TEXTURE_FORMATS:
        getCompressedFormats(params);
        break;
    case GL_MAX_CLIP_PLANES:
        ctx->dispatcher().glGetIntegerv(pname,params);
        if(*params > 6)
        {
            // GLES spec requires only 6, and the ATI driver erronously
            // returns 8 (although it supports only 6). This WAR is simple,
            // compliant and good enough for developers.
            *params = 6;
        }
        break;
    case GL_ALPHA_TEST_REF:
        // Both the ATI and nVidia OpenGL drivers return the wrong answer
        // here. So return the right one.
        ctx->dispatcher().glGetFloatv(pname,&f);
        *params = (int)(f * (float)0x7fffffff);
        break;
    case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
        ctx->dispatcher().glGetIntegerv(pname,params);
        if(*params > 16)
        {
            // GLES spec requires only 2, and the ATI driver erronously
            // returns 32 (although it supports only 16). This WAR is simple,
            // compliant and good enough for developers.
            *params = 16;
        }
        break;
    case GL_GENERATE_MIPMAP_HINT:
        if (isCoreProfile()) {
            *params = ctx->getHint(GL_GENERATE_MIPMAP_HINT);
        } else {
            ctx->dispatcher().glGetIntegerv(pname,params);
        }
        break;
    case GL_RED_BITS:
    case GL_GREEN_BITS:
    case GL_BLUE_BITS:
    case GL_ALPHA_BITS:
    case GL_DEPTH_BITS:
    case GL_STENCIL_BITS:
        if (isCoreProfile()) {
            GLuint fboBinding = ctx->getFramebufferBinding(GL_DRAW_FRAMEBUFFER);
            *params = ctx->queryCurrFboBits(fboBinding, pname);
        } else {
            ctx->dispatcher().glGetIntegerv(pname,params);
        }
        break;

    default:
        ctx->dispatcher().glGetIntegerv(pname,params);
    }
}

GL_API void GL_APIENTRY  glGetLightfv( GLenum light, GLenum pname, GLfloat *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->getLightfv(light,pname,params);
}

GL_API void GL_APIENTRY  glGetLightxv( GLenum light, GLenum pname, GLfixed *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    GLfloat tmpParams[4];

    ctx->getLightfv(light,pname,tmpParams);
    switch (pname){
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_POSITION:
            params[3] = F2X(tmpParams[3]);
        case GL_SPOT_DIRECTION:
            params[2] = F2X(tmpParams[2]);
        case GL_SPOT_EXPONENT:
        case GL_SPOT_CUTOFF:
        case GL_CONSTANT_ATTENUATION:
        case GL_LINEAR_ATTENUATION:
        case GL_QUADRATIC_ATTENUATION:
            params[1] = F2X(tmpParams[1]);
            break;
        default:{
            ctx->setGLerror(GL_INVALID_ENUM);
            return;
        }

    }
    params[0] = F2X(tmpParams[0]);
}

GL_API void GL_APIENTRY  glGetMaterialfv( GLenum face, GLenum pname, GLfloat *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->getMaterialfv(face,pname,params);
}

GL_API void GL_APIENTRY  glGetMaterialxv( GLenum face, GLenum pname, GLfixed *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    GLfloat tmpParams[4];
    ctx->getMaterialfv(face,pname,tmpParams);
    switch(pname){
    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_EMISSION:
    case GL_AMBIENT_AND_DIFFUSE:
        params[3] = tmpParams[3];
        params[2] = tmpParams[2];
        params[1] = tmpParams[1];
    case GL_SHININESS:
        params[0] = tmpParams[0];
        break;
    default:{
            ctx->setGLerror(GL_INVALID_ENUM);
            return;
        }
    }
}

GL_API void GL_APIENTRY  glGetPointerv( GLenum pname, void **params) {
    GET_CTX()
    GLES_CM_TRACE()
    const GLESpointer* p = ctx->getPointer(pname);
    if(p) {
        if (p->getAttribType() == GLESpointer::BUFFER) {
            *params = SafePointerFromUInt(p->getBufferOffset());
        } else if (p->getAttribType() == GLESpointer::ARRAY) {
            *params = const_cast<void *>(p->getArrayData());
        }
    } else {
        ctx->setGLerror(GL_INVALID_ENUM);
    }

}

GL_API void GL_APIENTRY  glGetTexEnvfv( GLenum env, GLenum pname, GLfloat *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()

    ctx->getTexEnvfv(env, pname, params);
}

GL_API void GL_APIENTRY  glGetTexEnviv( GLenum env, GLenum pname, GLint *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()

    ctx->getTexEnviv(env, pname, params);
}

GL_API void GL_APIENTRY  glGetTexEnvxv( GLenum env, GLenum pname, GLfixed *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    GLfloat tmpParams[4];

    ctx->getTexEnvfv(env, pname, tmpParams);

    if(pname == GL_TEXTURE_ENV_MODE) {
        params[0] = static_cast<GLfixed>(tmpParams[0]);
    } else {
        for(int i=0 ; i < 4 ; i++)
            params[i] = F2X(tmpParams[i]);
    }
}

GL_API void GL_APIENTRY  glGetTexParameterfv( GLenum target, GLenum pname, GLfloat *params) {
    GET_CTX()
    GLES_CM_TRACE()
   if (pname==GL_TEXTURE_CROP_RECT_OES) {
      TextureData *texData = getTextureTargetData(target);
      SET_ERROR_IF(texData==NULL,GL_INVALID_OPERATION);
      for (int i=0;i<4;++i)
        params[i] = texData->crop_rect[i];
    }
    else {
      ctx->dispatcher().glGetTexParameterfv(target,pname,params);
    }
}

GL_API void GL_APIENTRY  glGetTexParameteriv( GLenum target, GLenum pname, GLint *params) {
    GET_CTX()
    GLES_CM_TRACE()
    if (pname==GL_TEXTURE_CROP_RECT_OES) {
      TextureData *texData = getTextureTargetData(target);
      SET_ERROR_IF(texData==NULL,GL_INVALID_OPERATION);
      for (int i=0;i<4;++i)
        params[i] = texData->crop_rect[i];
    }
    else {
      ctx->dispatcher().glGetTexParameteriv(target,pname,params);
    }
}

GL_API void GL_APIENTRY  glGetTexParameterxv( GLenum target, GLenum pname, GLfixed *params) {
    GET_CTX()
    GLES_CM_TRACE()
    if (pname==GL_TEXTURE_CROP_RECT_OES) {
      TextureData *texData = getTextureTargetData(target);
      SET_ERROR_IF(texData==NULL,GL_INVALID_OPERATION);
      for (int i=0;i<4;++i)
        params[i] = F2X(texData->crop_rect[i]);
    }
    else {
      GLfloat tmpParam;
      ctx->dispatcher().glGetTexParameterfv(target,pname,&tmpParam);
      params[0] = static_cast<GLfixed>(tmpParam);
    }
}

GL_API void GL_APIENTRY  glHint( GLenum target, GLenum mode) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::hintTargetMode(target,mode),GL_INVALID_ENUM);

    // No GLES1 hints are supported.
    if (isGles2Gles() || isCoreProfile()) {
        ctx->setHint(target, mode);
    } else {
        ctx->dispatcher().glHint(target,mode);
    }
}

GL_API void GL_APIENTRY  glLightModelf( GLenum pname, GLfloat param) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->lightModelf(pname,param);
}

GL_API void GL_APIENTRY  glLightModelfv( GLenum pname, const GLfloat *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()

    ctx->lightModelfv(pname,params);
}

GL_API void GL_APIENTRY  glLightModelx( GLenum pname, GLfixed param) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    GLfloat tmpParam = static_cast<GLfloat>(param);
    ctx->lightModelf(pname,tmpParam);
}

GL_API void GL_APIENTRY  glLightModelxv( GLenum pname, const GLfixed *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    GLfloat tmpParams[4];
    if(pname == GL_LIGHT_MODEL_TWO_SIDE) {
        tmpParams[0] = X2F(params[0]);
    } else if (pname == GL_LIGHT_MODEL_AMBIENT) {
        for(int i=0;i<4;i++) {
            tmpParams[i] = X2F(params[i]);
        }
    }

    ctx->lightModelfv(pname,tmpParams);
}

GL_API void GL_APIENTRY  glLightf( GLenum light, GLenum pname, GLfloat param) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    gles1usages->set_light(true);
    ctx->lightf(light,pname,param);
}

GL_API void GL_APIENTRY  glLightfv( GLenum light, GLenum pname, const GLfloat *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->lightfv(light,pname,params);
}

GL_API void GL_APIENTRY  glLightx( GLenum light, GLenum pname, GLfixed param) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->lightf(light,pname,X2F(param));
}

GL_API void GL_APIENTRY  glLightxv( GLenum light, GLenum pname, const GLfixed *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    GLfloat tmpParams[4];

    switch (pname) {
        case GL_AMBIENT:
        case GL_DIFFUSE:
        case GL_SPECULAR:
        case GL_EMISSION:
        case GL_POSITION:
            tmpParams[3] = X2F(params[3]);
        case GL_SPOT_DIRECTION:
            tmpParams[2] = X2F(params[2]);
            tmpParams[1] = X2F(params[1]);
        case GL_SPOT_EXPONENT:
        case GL_SPOT_CUTOFF:
        case GL_CONSTANT_ATTENUATION:
        case GL_LINEAR_ATTENUATION:
        case GL_QUADRATIC_ATTENUATION:
            tmpParams[0] = X2F(params[0]);
            break;
        default: {
                ctx->setGLerror(GL_INVALID_ENUM);
                return;
            }
    }
    ctx->lightfv(light,pname,tmpParams);
}

GL_API void GL_APIENTRY  glLineWidth( GLfloat width) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setLineWidth(width);
    ctx->dispatcher().glLineWidth(width);
}

GL_API void GL_APIENTRY  glLineWidthx( GLfixed width) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setLineWidth(X2F(width));
    ctx->dispatcher().glLineWidth(X2F(width));
}

GL_API void GL_APIENTRY  glLoadIdentity( void) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->loadIdentity();
    ERRCHECK()
}

GL_API void GL_APIENTRY  glLoadMatrixf( const GLfloat *m) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->loadMatrixf(m);
}

GL_API void GL_APIENTRY  glLoadMatrixx( const GLfixed *m) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    GLfloat mat[16];
    for(int i=0; i< 16 ; i++) {
        mat[i] = X2F(m[i]);
    }
    ctx->loadMatrixf(mat);
}

GL_API void GL_APIENTRY  glLogicOp( GLenum opcode) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->dispatcher().glLogicOp(opcode);
}

GL_API void GL_APIENTRY  glMaterialf( GLenum face, GLenum pname, GLfloat param) {
    GET_CTX_CM()
    GLES_CM_TRACE()

    ctx->materialf(face, pname, param);
}

GL_API void GL_APIENTRY  glMaterialfv( GLenum face, GLenum pname, const GLfloat *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()

    ctx->materialfv(face, pname, params);
}

GL_API void GL_APIENTRY  glMaterialx( GLenum face, GLenum pname, GLfixed param) {
    GET_CTX_CM()
    GLES_CM_TRACE()

    ctx->materialf(face, pname, X2F(param));
}

GL_API void GL_APIENTRY  glMaterialxv( GLenum face, GLenum pname, const GLfixed *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    GLfloat tmpParams[4];

    for(int i=0; i< 4; i++) {
        tmpParams[i] = X2F(params[i]);
    }

    ctx->materialfv(face, pname, tmpParams);
}

GL_API void GL_APIENTRY  glMatrixMode( GLenum mode) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(mode != GL_TEXTURE &&
                 mode != GL_PROJECTION &&
                 mode != GL_MODELVIEW, GL_INVALID_ENUM);
    ERRCHECK()
    ctx->matrixMode(mode);
    ERRCHECK()
}

GL_API void GL_APIENTRY  glMultMatrixf( const GLfloat *m) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->multMatrixf(m);
}

GL_API void GL_APIENTRY  glMultMatrixx( const GLfixed *m) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    GLfloat mat[16];
    for(int i=0; i< 16 ; i++) {
        mat[i] = X2F(m[i]);
    }
    ctx->multMatrixf(mat);
}

GL_API void GL_APIENTRY  glMultiTexCoord4f( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::textureEnum(target,ctx->getMaxTexUnits()),GL_INVALID_ENUM);
    ctx->multiTexCoord4f(target, s, t, r, q);
}

GL_API void GL_APIENTRY  glMultiTexCoord4x( GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::textureEnum(target,ctx->getMaxTexUnits()),GL_INVALID_ENUM);
    ctx->multiTexCoord4f(target,X2F(s),X2F(t),X2F(r),X2F(q));
}

GL_API void GL_APIENTRY  glNormal3f( GLfloat nx, GLfloat ny, GLfloat nz) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->normal3f(nx, ny, nz);
}

GL_API void GL_APIENTRY  glNormal3x( GLfixed nx, GLfixed ny, GLfixed nz) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->normal3f(X2F(nx),X2F(ny),X2F(nz));
}

GL_API void GL_APIENTRY  glNormalPointer( GLenum type, GLsizei stride, const GLvoid *pointer) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(stride < 0,GL_INVALID_VALUE);
    SET_ERROR_IF(!GLEScmValidate::normalPointerType(type),GL_INVALID_ENUM);
    ctx->setPointer(GL_NORMAL_ARRAY,3,type,stride,pointer, 0);//3 normal verctor
}

GL_API void GL_APIENTRY glNormalPointerWithDataSize(GLenum type, GLsizei stride,
        const GLvoid *pointer, GLsizei dataSize) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(stride < 0,GL_INVALID_VALUE);
    SET_ERROR_IF(!GLEScmValidate::normalPointerType(type),GL_INVALID_ENUM);
    ctx->setPointer(GL_NORMAL_ARRAY,3,type,stride,pointer, dataSize);//3 normal verctor
}

GL_API void GL_APIENTRY  glOrthof( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->orthof(left,right,bottom,top,zNear,zFar);
}

GL_API void GL_APIENTRY  glOrthox( GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->orthof(left,right,bottom,top,zNear,zFar);
}

GL_API void GL_APIENTRY  glPixelStorei( GLenum pname, GLint param) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!(pname == GL_PACK_ALIGNMENT || pname == GL_UNPACK_ALIGNMENT),GL_INVALID_ENUM);
    SET_ERROR_IF(!((param==1)||(param==2)||(param==4)||(param==8)), GL_INVALID_VALUE);
    ctx->setPixelStorei(pname, param);
    ctx->dispatcher().glPixelStorei(pname,param);
}

GL_API void GL_APIENTRY  glPointParameterf( GLenum pname, GLfloat param) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->dispatcher().glPointParameterf(pname,param);
}

GL_API void GL_APIENTRY  glPointParameterfv( GLenum pname, const GLfloat *params) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->dispatcher().glPointParameterfv(pname,params);
}

GL_API void GL_APIENTRY  glPointParameterx( GLenum pname, GLfixed param)
{
    GET_CTX()
    GLES_CM_TRACE()
    ctx->dispatcher().glPointParameterf(pname,X2F(param));
}

GL_API void GL_APIENTRY  glPointParameterxv( GLenum pname, const GLfixed *params) {
    GET_CTX()
    GLES_CM_TRACE()

    GLfloat tmpParam = X2F(*params) ;
    ctx->dispatcher().glPointParameterfv(pname,&tmpParam);
}

GL_API void GL_APIENTRY  glPointSize( GLfloat size) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->dispatcher().glPointSize(size);
}

GL_API void GL_APIENTRY  glPointSizePointerOES( GLenum type, GLsizei stride, const GLvoid *pointer) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(stride < 0,GL_INVALID_VALUE);
    SET_ERROR_IF(!GLEScmValidate::pointPointerType(type),GL_INVALID_ENUM);
    ctx->setPointer(GL_POINT_SIZE_ARRAY_OES,1,type,stride,pointer, 0);
}

GL_API void GL_APIENTRY  glPointSizex( GLfixed size) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->dispatcher().glPointSize(X2F(size));
}

GL_API void GL_APIENTRY  glPolygonOffset( GLfloat factor, GLfloat units) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setPolygonOffset(factor, units);
    ctx->dispatcher().glPolygonOffset(factor,units);
}

GL_API void GL_APIENTRY  glPolygonOffsetx( GLfixed factor, GLfixed units) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setPolygonOffset(X2F(factor), X2F(units));
    ctx->dispatcher().glPolygonOffset(X2F(factor),X2F(units));
}

GL_API void GL_APIENTRY  glPopMatrix(void) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->popMatrix();
    CORE_ERR_FORWARD()
}

GL_API void GL_APIENTRY  glPushMatrix(void) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->pushMatrix();
    CORE_ERR_FORWARD()
}

GL_API void GL_APIENTRY  glReadPixels( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {
    GET_CTX()
    GLES_CM_TRACE()

    SET_ERROR_IF(!(GLEScmValidate::pixelFrmt(ctx,format) && GLEScmValidate::pixelType(ctx,type)),GL_INVALID_ENUM);
    SET_ERROR_IF(!(GLEScmValidate::pixelOp(format,type)),GL_INVALID_OPERATION);

    // Just stop allowing glReadPixels on multisampled default FBO for now.
    SET_ERROR_IF(ctx->isDefaultFBOBound(GL_FRAMEBUFFER_EXT) &&
                 ctx->getDefaultFBOMultisamples(), GL_INVALID_OPERATION);

    ctx->dispatcher().glReadPixels(x,y,width,height,format,type,pixels);
}

GL_API void GL_APIENTRY  glRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->rotatef(angle, x, y, z);
}

GL_API void GL_APIENTRY  glRotatex( GLfixed angle, GLfixed x, GLfixed y, GLfixed z) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->rotatef(angle,X2F(x),X2F(y),X2F(z));
}

GL_API void GL_APIENTRY  glSampleCoverage( GLclampf value, GLboolean invert) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setSampleCoverage(value, invert);
    ctx->dispatcher().glSampleCoverage(value,invert);
}

GL_API void GL_APIENTRY  glSampleCoveragex( GLclampx value, GLboolean invert) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setSampleCoverage(X2F(value), invert);
    ctx->dispatcher().glSampleCoverage(X2F(value),invert);
}

GL_API void GL_APIENTRY  glScalef( GLfloat x, GLfloat y, GLfloat z) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->scalef(x, y, z);
}

GL_API void GL_APIENTRY  glScalex( GLfixed x, GLfixed y, GLfixed z) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->scalef(X2F(x),X2F(y),X2F(z));
}

GL_API void GL_APIENTRY  glScissor( GLint x, GLint y, GLsizei width, GLsizei height) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setScissor(x, y, width, height);
    ctx->dispatcher().glScissor(x,y,width,height);
}

GL_API void GL_APIENTRY  glShadeModel( GLenum mode ) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->shadeModel(mode);
}

GL_API void GL_APIENTRY  glStencilFunc( GLenum func, GLint ref, GLuint mask) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setStencilFuncSeparate(GL_FRONT_AND_BACK, func, ref, mask);
    ctx->dispatcher().glStencilFunc(func,ref,mask);
}

GL_API void GL_APIENTRY  glStencilMask( GLuint mask) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setStencilMaskSeparate(GL_FRONT_AND_BACK, mask);
    ctx->dispatcher().glStencilMask(mask);
}

GL_API void GL_APIENTRY  glStencilOp( GLenum fail, GLenum zfail, GLenum zpass) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!(GLEScmValidate::stencilOp(fail) && GLEScmValidate::stencilOp(zfail) && GLEScmValidate::stencilOp(zpass)),GL_INVALID_ENUM);
    ctx->setStencilOpSeparate(GL_FRONT_AND_BACK, fail, zfail, zpass);
    ctx->dispatcher().glStencilOp(fail,zfail,zpass);
}

GL_API void GL_APIENTRY  glTexCoordPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texCoordPointerParams(size,stride),GL_INVALID_VALUE);
    SET_ERROR_IF(!GLEScmValidate::texCoordPointerType(type),GL_INVALID_ENUM);
    ctx->setPointer(GL_TEXTURE_COORD_ARRAY,size,type,stride,pointer, 0);
}

GL_API void GL_APIENTRY  glTexCoordPointerWithDataSize( GLint size, GLenum type,
        GLsizei stride, const GLvoid *pointer, GLsizei dataSize) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texCoordPointerParams(size,stride),GL_INVALID_VALUE);
    SET_ERROR_IF(!GLEScmValidate::texCoordPointerType(type),GL_INVALID_ENUM);
    ctx->setPointer(GL_TEXTURE_COORD_ARRAY,size,type,stride,pointer, dataSize);
}

GL_API void GL_APIENTRY  glTexEnvf( GLenum target, GLenum pname, GLfloat param) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texEnv(target,pname),GL_INVALID_ENUM);
    ctx->texEnvf(target,pname,param);
}

GL_API void GL_APIENTRY  glTexEnvfv( GLenum target, GLenum pname, const GLfloat *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texEnv(target,pname),GL_INVALID_ENUM);
    ctx->texEnvfv(target, pname, params);
}

GL_API void GL_APIENTRY  glTexEnvi( GLenum target, GLenum pname, GLint param) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texEnv(target,pname),GL_INVALID_ENUM);
    ctx->texEnvi(target, pname, param);
}

GL_API void GL_APIENTRY  glTexEnviv( GLenum target, GLenum pname, const GLint *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texEnv(target,pname),GL_INVALID_ENUM);
    ctx->texEnviv(target, pname, params);
}

GL_API void GL_APIENTRY  glTexEnvx( GLenum target, GLenum pname, GLfixed param) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texEnv(target,pname),GL_INVALID_ENUM);

    GLfloat tmpParam = static_cast<GLfloat>(param);
    ctx->texEnvf(target, pname, tmpParam);
    CORE_ERR_FORWARD()
}

GL_API void GL_APIENTRY  glTexEnvxv( GLenum target, GLenum pname, const GLfixed *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texEnv(target,pname),GL_INVALID_ENUM);

    GLfloat tmpParams[4];
    if(pname == GL_TEXTURE_ENV_COLOR) {
        for(int i =0;i<4;i++) {
            tmpParams[i] = X2F(params[i]);
        }
    } else {
        tmpParams[0] = static_cast<GLfloat>(params[0]);
    }

    ctx->texEnvfv(target, pname, tmpParams);
    CORE_ERR_FORWARD()
}

GL_API void GL_APIENTRY  glTexImage2D( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    GET_CTX()
    GLES_CM_TRACE()

    SET_ERROR_IF(!(GLEScmValidate::textureTargetEx(target) &&
                     GLEScmValidate::pixelFrmt(ctx,internalformat) &&
                     GLEScmValidate::pixelFrmt(ctx,format) &&
                     GLEScmValidate::pixelType(ctx,type)),GL_INVALID_ENUM);

    SET_ERROR_IF(!(GLEScmValidate::pixelOp(format,type) && internalformat == ((GLint)format)),GL_INVALID_OPERATION);

    bool needAutoMipmap = false;

    s_glInitTexImage2D(target, level, internalformat, width, height, border,
            &format, &type, &internalformat, &needAutoMipmap);

    // TODO: Emulate swizzles
    if (isCoreProfile()) {
        GLEScontext::prepareCoreProfileEmulatedTexture(
            getTextureTargetData(target),
            false, target, format, type,
            &internalformat, &format);
    }

    ctx->dispatcher().glTexImage2D(target,level,
                                   internalformat,width,height,
                                   border,format,type,pixels);

    if (needAutoMipmap) {
        if ((isCoreProfile() || isGles2Gles()) &&
            !isCubeMapFaceTarget(target)) {
            ctx->dispatcher().glGenerateMipmap(target);
        } else if (isGles2Gles()) {
            ctx->dispatcher().glGenerateMipmap(target);
        } else {
            ctx->dispatcher().glGenerateMipmapEXT(target);
        }
    }
}

static bool handleMipmapGeneration(GLenum target, GLenum pname, bool param)
{
    GET_CTX_RET(false)
    GLES_CM_TRACE()

    if (pname == GL_GENERATE_MIPMAP) {
        TextureData *texData = getTextureTargetData(target);
        if (texData) {
            if (param) {
                texData->setMipmapLevelAtLeast(maxMipmapLevel(texData->width,
                    texData->height));
            }
            if (isCoreProfile() || isGles2Gles() ||
                    !ctx->isAutoMipmapSupported()) {
                texData->requiresAutoMipmap = param;
                return true;
            }
        }
    }

    return false;
}

GL_API void GL_APIENTRY  glTexParameterf( GLenum target, GLenum pname, GLfloat param) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texParams(target,pname),GL_INVALID_ENUM);

    if(handleMipmapGeneration(target, pname, (bool)param))
        return;

    TextureData *texData = getTextureTargetData(target);
    texData->setTexParam(pname, static_cast<GLint>(param));
    ctx->dispatcher().glTexParameterf(target,pname,param);
}

GL_API void GL_APIENTRY  glTexParameterfv( GLenum target, GLenum pname, const GLfloat *params) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texParams(target,pname),GL_INVALID_ENUM);

    if(handleMipmapGeneration(target, pname, (bool)(*params)))
        return;

    TextureData *texData = getTextureTargetData(target);

    if (pname==GL_TEXTURE_CROP_RECT_OES) {
        SET_ERROR_IF(texData==NULL,GL_INVALID_OPERATION);
        for (int i=0;i<4;++i)
            texData->crop_rect[i] = params[i];
    } else {
        texData->setTexParam(pname, static_cast<GLint>(params[0]));
        ctx->dispatcher().glTexParameterfv(target,pname,params);
    }
}

GL_API void GL_APIENTRY  glTexParameteri( GLenum target, GLenum pname, GLint param) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texParams(target,pname),GL_INVALID_ENUM);

    if(handleMipmapGeneration(target, pname, (bool)param))
        return;

    TextureData *texData = getTextureTargetData(target);
    texData->setTexParam(pname, (GLint)param);
    ctx->dispatcher().glTexParameteri(target,pname,param);
}

GL_API void GL_APIENTRY  glTexParameteriv( GLenum target, GLenum pname, const GLint *params) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texParams(target,pname),GL_INVALID_ENUM);

    if(handleMipmapGeneration(target, pname, (bool)(*params)))
        return;

    TextureData *texData = getTextureTargetData(target);
    if (pname==GL_TEXTURE_CROP_RECT_OES) {
        SET_ERROR_IF(texData==NULL,GL_INVALID_OPERATION);
        for (int i=0;i<4;++i)
            texData->crop_rect[i] = params[i];
    }
    else {
        texData->setTexParam(pname, (GLint)params[0]);
        ctx->dispatcher().glTexParameteriv(target,pname,params);
    }
}

GL_API void GL_APIENTRY  glTexParameterx( GLenum target, GLenum pname, GLfixed param) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texParams(target,pname),GL_INVALID_ENUM);
    if(handleMipmapGeneration(target, pname, (bool)param))
        return;

    TextureData *texData = getTextureTargetData(target);
    texData->setTexParam(pname, static_cast<GLint>(param));
    ctx->dispatcher().glTexParameterf(target,pname,static_cast<GLfloat>(param));
}

GL_API void GL_APIENTRY  glTexParameterxv( GLenum target, GLenum pname, const GLfixed *params) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texParams(target,pname),GL_INVALID_ENUM);

    if(handleMipmapGeneration(target, pname, (bool)(*params)))
        return;

    TextureData *texData = getTextureTargetData(target);
    if (pname==GL_TEXTURE_CROP_RECT_OES) {
        SET_ERROR_IF(texData==NULL,GL_INVALID_OPERATION);
        for (int i=0;i<4;++i)
            texData->crop_rect[i] = X2F(params[i]);
    }
    else {
        GLfloat param = static_cast<GLfloat>(params[0]);
        texData->setTexParam(pname, static_cast<GLint>(params[0]));
        ctx->dispatcher().glTexParameterfv(target,pname,&param);
    }
}

GL_API void GL_APIENTRY  glTexSubImage2D( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!(GLEScmValidate::textureTargetEx(target) &&
                   GLEScmValidate::pixelFrmt(ctx,format)&&
                   GLEScmValidate::pixelType(ctx,type)),GL_INVALID_ENUM);
    SET_ERROR_IF(!GLEScmValidate::pixelOp(format,type),GL_INVALID_OPERATION);
    // set an error if level < 0 or level > log 2 max
    SET_ERROR_IF(level < 0 || 1<<level > ctx->getMaxTexSize(), GL_INVALID_VALUE);
    SET_ERROR_IF(xoffset < 0 || yoffset < 0 || width < 0 || height < 0, GL_INVALID_VALUE);
    if (ctx->shareGroup().get()) {
        TextureData *texData = getTextureTargetData(target);
        SET_ERROR_IF(!texData, GL_INVALID_OPERATION);
        SET_ERROR_IF(xoffset + width > (GLint)texData->width ||
                 yoffset + height > (GLint)texData->height,
                 GL_INVALID_VALUE);
    }
    SET_ERROR_IF(!pixels,GL_INVALID_OPERATION);

    ctx->dispatcher().glTexSubImage2D(target,level,xoffset,yoffset,width,height,format,type,pixels);

    if (ctx->shareGroup().get()){
        TextureData *texData = getTextureTargetData(target);
        if(texData && texData->requiresAutoMipmap)
        {
            ctx->dispatcher().glGenerateMipmapEXT(target);
        }
        texData->setMipmapLevelAtLeast(level);
        texData->makeDirty();
    }
}

GL_API void GL_APIENTRY  glTranslatef( GLfloat x, GLfloat y, GLfloat z) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->translatef(x, y, z);
}

GL_API void GL_APIENTRY  glTranslatex( GLfixed x, GLfixed y, GLfixed z) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->translatef(X2F(x),X2F(y),X2F(z));
}

GL_API void GL_APIENTRY  glVertexPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    GET_CTX()
    GLES_CM_TRACE()

    SET_ERROR_IF(!GLEScmValidate::vertexPointerParams(size,stride),GL_INVALID_VALUE);
    SET_ERROR_IF(!GLEScmValidate::vertexPointerType(type),GL_INVALID_ENUM);
    ctx->setPointer(GL_VERTEX_ARRAY,size,type,stride,pointer, 0);
}

GL_API void GL_APIENTRY  glVertexPointerWithDataSize( GLint size, GLenum type,
        GLsizei stride, const GLvoid *pointer, GLsizei dataSize) {
    GET_CTX()
    GLES_CM_TRACE()

    SET_ERROR_IF(!GLEScmValidate::vertexPointerParams(size,stride),GL_INVALID_VALUE);
    SET_ERROR_IF(!GLEScmValidate::vertexPointerType(type),GL_INVALID_ENUM);
    ctx->setPointer(GL_VERTEX_ARRAY,size,type,stride,pointer, dataSize);
}

GL_API void GL_APIENTRY  glViewport( GLint x, GLint y, GLsizei width, GLsizei height) {
    GET_CTX()
    GLES_CM_TRACE()
    ctx->setViewport(x, y, width, height);
    ctx->dispatcher().glViewport(x,y,width,height);
}

GL_API void GL_APIENTRY glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image)
{
    GET_CTX();
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::textureTargetLimited(target),GL_INVALID_ENUM);
    unsigned int imagehndl = SafeUIntFromPointer(image);
    ImagePtr img = s_eglIface->getEGLImage(imagehndl);
    if (img) {
        // Create the texture object in the underlying EGL implementation,
        // flag to the OpenGL layer to skip the image creation and map the
        // current binded texture object to the existing global object.
        if (ctx->shareGroup().get()) {
            ObjectLocalName tex = ctx->getTextureLocalName(target,ctx->getBindedTexture(target));
            // replace mapping and bind the new global object
            ctx->shareGroup()->replaceGlobalObject(NamedObjectType::TEXTURE, tex,
                                                   img->globalTexObj);
            ctx->dispatcher().glBindTexture(GL_TEXTURE_2D,
                                            img->globalTexObj->getGlobalName());
            TextureData *texData = getTextureTargetData(target);
            SET_ERROR_IF(texData==NULL,GL_INVALID_OPERATION);
            texData->width = img->width;
            texData->height = img->height;
            texData->border = img->border;
            texData->internalFormat = img->internalFormat;
            texData->format = img->format;
            texData->type = img->type;
            texData->texStorageLevels = img->texStorageLevels;
            texData->sourceEGLImage = imagehndl;
            texData->setGlobalName(img->globalTexObj->getGlobalName());
            texData->setSaveableTexture(
                    SaveableTexturePtr(img->saveableTexture));
            if (img->sync) {
                // insert gpu side fence to make sure we are done with any blit ops.
                ctx->dispatcher().glWaitSync(img->sync, 0, GL_TIMEOUT_IGNORED);
            }
        }
    }
}

GL_API void GL_APIENTRY glEGLImageTargetRenderbufferStorageOES(GLenum target, GLeglImageOES image)
{
    GET_CTX();
    GLES_CM_TRACE()
    SET_ERROR_IF(target != GL_RENDERBUFFER_OES,GL_INVALID_ENUM);
    unsigned int imagehndl = SafeUIntFromPointer(image);
    ImagePtr img = s_eglIface->getEGLImage(imagehndl);
    SET_ERROR_IF(!img,GL_INVALID_VALUE);
    SET_ERROR_IF(!ctx->shareGroup().get(),GL_INVALID_OPERATION);

    // Get current bounded renderbuffer
    // raise INVALID_OPERATIOn if no renderbuffer is bounded
    GLuint rb = ctx->getRenderbufferBinding();
    SET_ERROR_IF(rb == 0,GL_INVALID_OPERATION);
    auto objData =
            ctx->shareGroup()->getObjectData(NamedObjectType::RENDERBUFFER, rb);
    RenderbufferData *rbData = (RenderbufferData *)objData;
    SET_ERROR_IF(!rbData,GL_INVALID_OPERATION);

    //
    // acquire the texture in the renderbufferData that it is an eglImage target
    //
    rbData->eglImageGlobalTexObject = img->globalTexObj;
    rbData->saveableTexture = img->saveableTexture;
    img->saveableTexture->makeDirty();

    //
    // if the renderbuffer is attached to a framebuffer
    // change the framebuffer attachment in the undelying OpenGL
    // to point to the eglImage texture object.
    //
    if (rbData->attachedFB) {
        // update the framebuffer attachment point to the
        // underlying texture of the img
        GLuint prevFB = ctx->getFramebufferBinding(GL_FRAMEBUFFER_EXT);
        if (prevFB != rbData->attachedFB) {
            if (isCoreProfile()) {
                ctx->dispatcher().glBindFramebuffer(GL_FRAMEBUFFER,
                        rbData->attachedFB);
            } else {
                ctx->dispatcher().glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,
                        rbData->attachedFB);
            }
        }
        if (isCoreProfile()) {
            ctx->dispatcher().glFramebufferTexture2D(GL_FRAMEBUFFER,
                    rbData->attachedPoint,
                    GL_TEXTURE_2D,
                    img->globalTexObj->getGlobalName(),
                    0);
        } else {
            ctx->dispatcher().glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                    rbData->attachedPoint,
                    GL_TEXTURE_2D,
                    img->globalTexObj->getGlobalName(),
                    0);
        }
        if (prevFB != rbData->attachedFB) {
            if (isCoreProfile()) {
                ctx->dispatcher().glBindFramebuffer(GL_FRAMEBUFFER,
                                                    prevFB);
            } else {
                ctx->dispatcher().glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,
                                                       prevFB);
            }
        }
    }
}

/* GL_OES_blend_subtract*/
GL_API void GL_APIENTRY glBlendEquationOES(GLenum mode) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!(GLEScmValidate::blendEquationMode(mode)), GL_INVALID_ENUM);
    ctx->setBlendEquationSeparate(mode, mode);
    ctx->dispatcher().glBlendEquation(mode);
}

/* GL_OES_blend_equation_separate */
GL_API void GL_APIENTRY glBlendEquationSeparateOES (GLenum modeRGB, GLenum modeAlpha) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!(GLEScmValidate::blendEquationMode(modeRGB) && GLEScmValidate::blendEquationMode(modeAlpha)), GL_INVALID_ENUM);
    ctx->setBlendEquationSeparate(modeRGB, modeAlpha);
    ctx->dispatcher().glBlendEquationSeparate(modeRGB,modeAlpha);
}

/* GL_OES_blend_func_separate */
GL_API void GL_APIENTRY glBlendFuncSeparateOES(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::blendSrc(srcRGB) || !GLEScmValidate::blendDst(dstRGB) ||
                 !GLEScmValidate::blendSrc(srcAlpha) || ! GLEScmValidate::blendDst(dstAlpha) ,GL_INVALID_ENUM);
    ctx->setBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha);
    ctx->dispatcher().glBlendFuncSeparate(srcRGB,dstRGB,srcAlpha,dstAlpha);
}

/* GL_OES_framebuffer_object */
GL_API GLboolean GL_APIENTRY glIsRenderbufferOES(GLuint renderbuffer) {
    GET_CTX_RET(GL_FALSE)
    GLES_CM_TRACE()
    RET_AND_SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION,GL_FALSE);
    if(renderbuffer && ctx->shareGroup().get()){
        return ctx->shareGroup()->isObject(NamedObjectType::RENDERBUFFER,
                                           renderbuffer)
                       ? GL_TRUE
                       : GL_FALSE;
    }
    if (isCoreProfile()) {
        return ctx->dispatcher().glIsRenderbuffer(renderbuffer);
    } else {
        return ctx->dispatcher().glIsRenderbufferEXT(renderbuffer);
    }
}

GL_API void GLAPIENTRY glBindRenderbufferOES(GLenum target, GLuint renderbuffer) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
    SET_ERROR_IF(!GLEScmValidate::renderbufferTarget(target),GL_INVALID_ENUM);

    //if buffer wasn't generated before,generate one
    if (renderbuffer && ctx->shareGroup().get() &&
        !ctx->shareGroup()->isObject(NamedObjectType::RENDERBUFFER,
                                     renderbuffer)) {
        ctx->shareGroup()->genName(NamedObjectType::RENDERBUFFER, renderbuffer);
        ctx->shareGroup()->setObjectData(NamedObjectType::RENDERBUFFER,
                                         renderbuffer,
                                         ObjectDataPtr(new RenderbufferData()));
    }

    int globalBufferName =
            (renderbuffer != 0)
                    ? ctx->shareGroup()->getGlobalName(
                              NamedObjectType::RENDERBUFFER, renderbuffer)
                    : 0;
    if (isCoreProfile()) {
        ctx->dispatcher().glBindRenderbuffer(target,globalBufferName);
    } else {
        ctx->dispatcher().glBindRenderbufferEXT(target,globalBufferName);
    }

    // update renderbuffer binding state
    ctx->setRenderbufferBinding(renderbuffer);
}

GL_API void GLAPIENTRY glDeleteRenderbuffersOES(GLsizei n, const GLuint *renderbuffers) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
    for (int i=0;i<n;++i) {
        ctx->shareGroup()->deleteName(NamedObjectType::RENDERBUFFER,
                                      renderbuffers[i]);
    }
}

GL_API void GLAPIENTRY glGenRenderbuffersOES(GLsizei n, GLuint *renderbuffers) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    if(ctx->shareGroup().get()) {
        for(int i=0; i<n ;i++) {
            renderbuffers[i] = ctx->shareGroup()->genName(
                    NamedObjectType::RENDERBUFFER, 0, true);
            ctx->shareGroup()->setObjectData(
                    NamedObjectType::RENDERBUFFER, renderbuffers[i],
                    ObjectDataPtr(new RenderbufferData()));
        }
    }
}

GL_API void GLAPIENTRY glRenderbufferStorageOES(GLenum target, GLenum internalformat, GLsizei width, GLsizei height){
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
    SET_ERROR_IF(!GLEScmValidate::renderbufferTarget(target) || !GLEScmValidate::renderbufferInternalFrmt(ctx,internalformat) ,GL_INVALID_ENUM);
    if (internalformat==GL_RGB565_OES) //RGB565 not supported by GL
        internalformat = GL_RGB8_OES;

    // Get current bounded renderbuffer
    // raise INVALID_OPERATIOn if no renderbuffer is bounded
    GLuint rb = ctx->getRenderbufferBinding();
    SET_ERROR_IF(rb == 0,GL_INVALID_OPERATION);
    auto objData = ctx->shareGroup()->getObjectData(NamedObjectType::RENDERBUFFER, rb);
    RenderbufferData *rbData = (RenderbufferData *)objData;
    SET_ERROR_IF(!rbData,GL_INVALID_OPERATION);

    //
    // if the renderbuffer was an eglImage target, release
    // its underlying texture.
    //
    rbData->eglImageGlobalTexObject.reset();
    rbData->saveableTexture.reset();

    ctx->dispatcher().glRenderbufferStorageEXT(target,internalformat,width,height);
}

GL_API void GLAPIENTRY glGetRenderbufferParameterivOES(GLenum target, GLenum pname, GLint* params) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
    SET_ERROR_IF(!GLEScmValidate::renderbufferTarget(target) || !GLEScmValidate::renderbufferParams(pname) ,GL_INVALID_ENUM);

    //
    // If this is a renderbuffer which is eglimage's target, we
    // should query the underlying eglimage's texture object instead.
    //
    GLuint rb = ctx->getRenderbufferBinding();
    if (rb) {
        auto objData = ctx->shareGroup()->getObjectData(
                NamedObjectType::RENDERBUFFER, rb);
        RenderbufferData *rbData = (RenderbufferData *)objData;
        if (rbData && rbData->eglImageGlobalTexObject) {
            GLenum texPname;
            switch(pname) {
                case GL_RENDERBUFFER_WIDTH_OES:
                    texPname = GL_TEXTURE_WIDTH;
                    break;
                case GL_RENDERBUFFER_HEIGHT_OES:
                    texPname = GL_TEXTURE_HEIGHT;
                    break;
                case GL_RENDERBUFFER_INTERNAL_FORMAT_OES:
                    texPname = GL_TEXTURE_INTERNAL_FORMAT;
                    break;
                case GL_RENDERBUFFER_RED_SIZE_OES:
                    texPname = GL_TEXTURE_RED_SIZE;
                    break;
                case GL_RENDERBUFFER_GREEN_SIZE_OES:
                    texPname = GL_TEXTURE_GREEN_SIZE;
                    break;
                case GL_RENDERBUFFER_BLUE_SIZE_OES:
                    texPname = GL_TEXTURE_BLUE_SIZE;
                    break;
                case GL_RENDERBUFFER_ALPHA_SIZE_OES:
                    texPname = GL_TEXTURE_ALPHA_SIZE;
                    break;
                case GL_RENDERBUFFER_DEPTH_SIZE_OES:
                    texPname = GL_TEXTURE_DEPTH_SIZE;
                    break;
                case GL_RENDERBUFFER_STENCIL_SIZE_OES:
                default:
                    *params = 0; //XXX
                    return;
                    break;
            }

            GLint prevTex;
            ctx->dispatcher().glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
            ctx->dispatcher().glBindTexture(GL_TEXTURE_2D,
                                            rbData->eglImageGlobalTexObject->getGlobalName());
            ctx->dispatcher().glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
                                                       texPname,
                                                       params);
            ctx->dispatcher().glBindTexture(GL_TEXTURE_2D, prevTex);
            return;
        }
    }

    ctx->dispatcher().glGetRenderbufferParameterivEXT(target,pname,params);
}

GL_API GLboolean GLAPIENTRY glIsFramebufferOES(GLuint framebuffer) {
    GET_CTX_RET(GL_FALSE)
    GLES_CM_TRACE()
    RET_AND_SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION,GL_FALSE);
    if (framebuffer) {
        return ctx->isFBO(framebuffer)
                       ? GL_TRUE
                       : GL_FALSE;
    }
    if (isCoreProfile()) {
        return ctx->dispatcher().glIsFramebuffer(framebuffer);
    } else {
        return ctx->dispatcher().glIsFramebufferEXT(framebuffer);
    }
}

GL_API void GLAPIENTRY glBindFramebufferOES(GLenum target, GLuint framebuffer) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
    SET_ERROR_IF(!GLEScmValidate::framebufferTarget(target) ,GL_INVALID_ENUM);
    if (framebuffer && !ctx->isFBO(framebuffer)) {
        ctx->genFBOName(framebuffer);
        ctx->setFBOData(framebuffer,
                ObjectDataPtr(
                    new FramebufferData(
                            framebuffer,
                            ctx->getFBOGlobalName(framebuffer))));
    }
    int globalBufferName =
            (framebuffer != 0)
                    ? ctx->getFBOGlobalName(framebuffer)
                    : ctx->getDefaultFBOGlobalName();
    if (isCoreProfile()) {
        ctx->dispatcher().glBindFramebuffer(target,globalBufferName);
    } else {
        ctx->dispatcher().glBindFramebufferEXT(target,globalBufferName);
    }

    // update framebuffer binding state
    ctx->setFramebufferBinding(GL_FRAMEBUFFER_EXT, framebuffer);
}

GL_API void GLAPIENTRY glDeleteFramebuffersOES(GLsizei n, const GLuint *framebuffers) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
    GLuint fbName = ctx->getFramebufferBinding(GL_FRAMEBUFFER_EXT);
    for (int i=0;i<n;++i) {
        if (framebuffers[i] == fbName)
            glBindFramebufferOES(GL_FRAMEBUFFER_EXT, 0);
        ctx->deleteFBO(framebuffers[i]);
    }
}

GL_API void GLAPIENTRY glGenFramebuffersOES(GLsizei n, GLuint *framebuffers) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
    SET_ERROR_IF(n<0,GL_INVALID_VALUE);
    for (int i=0;i<n;i++) {
        framebuffers[i] = ctx->genFBOName(0, true);
        ctx->setFBOData(framebuffers[i],
                        ObjectDataPtr(
                            new FramebufferData(
                                framebuffers[i],
                                ctx->getFBOGlobalName(framebuffers[i]))));
    }
}

GL_API GLenum GLAPIENTRY glCheckFramebufferStatusOES(GLenum target) {
    GET_CTX_RET(0)
    GLES_CM_TRACE()
    RET_AND_SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION,0);
    RET_AND_SET_ERROR_IF(!GLEScmValidate::framebufferTarget(target) ,GL_INVALID_ENUM,0);
    return ctx->dispatcher().glCheckFramebufferStatusEXT(target);
}

GL_API void GLAPIENTRY glFramebufferTexture2DOES(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
    SET_ERROR_IF(!GLEScmValidate::framebufferTarget(target) || !GLEScmValidate::framebufferAttachment(attachment) ||
                 !GLEScmValidate::textureTargetEx(textarget),GL_INVALID_ENUM);
    SET_ERROR_IF(!ctx->shareGroup().get(), GL_INVALID_OPERATION);
    SET_ERROR_IF(ctx->isDefaultFBOBound(target), GL_INVALID_OPERATION);

    GLuint globalTexName = 0;
    if(texture) {
        if (!ctx->shareGroup()->isObject(NamedObjectType::TEXTURE, texture)) {
            ctx->shareGroup()->genName(NamedObjectType::TEXTURE, texture);
        }
        ObjectLocalName texname = ctx->getTextureLocalName(textarget,texture);
        globalTexName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::TEXTURE, texname);
    }

    ctx->dispatcher().glFramebufferTexture2DEXT(target,attachment,textarget,globalTexName,level);

    // Update the the current framebuffer object attachment state
    GLuint fbName = ctx->getFramebufferBinding(GL_FRAMEBUFFER_EXT);
    auto fbObj = ctx->getFBOData(fbName);
    if (fbObj) {
        fbObj->setAttachment(
            ctx, attachment, textarget,
            texture, ObjectDataPtr());
    }
}

GL_API void GLAPIENTRY glFramebufferRenderbufferOES(GLenum target, GLenum attachment,GLenum renderbuffertarget, GLuint renderbuffer) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
    SET_ERROR_IF(!GLEScmValidate::framebufferTarget(target) ||
                 !GLEScmValidate::framebufferAttachment(attachment) ||
                 !GLEScmValidate::renderbufferTarget(renderbuffertarget), GL_INVALID_ENUM);
    SET_ERROR_IF(!ctx->shareGroup().get(), GL_INVALID_OPERATION);
    SET_ERROR_IF(ctx->isDefaultFBOBound(target), GL_INVALID_OPERATION);

    GLuint globalBufferName = 0;
    ObjectDataPtr obj;

    // generate the renderbuffer object if not yet exist
    if (renderbuffer) {
        if (!ctx->shareGroup()->isObject(NamedObjectType::RENDERBUFFER,
                                         renderbuffer)) {
            ctx->shareGroup()->genName(NamedObjectType::RENDERBUFFER,
                                       renderbuffer);
            obj = ObjectDataPtr(new RenderbufferData());
            ctx->shareGroup()->setObjectData(
                    NamedObjectType::RENDERBUFFER, renderbuffer, obj);
        }
        else {
            obj = ctx->shareGroup()->getObjectDataPtr(
                    NamedObjectType::RENDERBUFFER, renderbuffer);
        }
        globalBufferName = ctx->shareGroup()->getGlobalName(
                NamedObjectType::RENDERBUFFER, renderbuffer);
    }

    // Update the the current framebuffer object attachment state
    GLuint fbName = ctx->getFramebufferBinding(GL_FRAMEBUFFER_EXT);
    auto fbObj = ctx->getFBOData(fbName);
    if (fbObj) {
        fbObj->setAttachment(
            ctx, attachment, renderbuffertarget, renderbuffer, obj);
    }

    if (renderbuffer && obj.get() != NULL) {
        RenderbufferData *rbData = (RenderbufferData *)obj.get();
        if (rbData->eglImageGlobalTexObject) {
            //
            // This renderbuffer object is an eglImage target
            // attach the eglimage's texture instead the renderbuffer.
            //
            ctx->dispatcher().glFramebufferTexture2DEXT(target,
                                                attachment,
                                                GL_TEXTURE_2D,
                                                rbData->eglImageGlobalTexObject->getGlobalName(),
                                                0);
            return;
        }
    }

    if (isCoreProfile()) {
        ctx->dispatcher().glFramebufferRenderbuffer(target,attachment,renderbuffertarget,globalBufferName);
    } else {
        ctx->dispatcher().glFramebufferRenderbufferEXT(target,attachment,renderbuffertarget,globalBufferName);
    }
}

GL_API void GLAPIENTRY glGetFramebufferAttachmentParameterivOES(GLenum target, GLenum attachment, GLenum pname, GLint *params) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
    SET_ERROR_IF(!GLEScmValidate::framebufferTarget(target) || !GLEScmValidate::framebufferAttachment(attachment) ||
                 !GLEScmValidate::framebufferAttachmentParams(pname), GL_INVALID_ENUM);

    //
    // Take the attachment attribute from our state - if available
    //
    GLuint fbName = ctx->getFramebufferBinding(GL_FRAMEBUFFER_EXT);
    if (fbName) {
        auto fbObj = ctx->getFBOData(fbName);
        if (fbObj) {
            GLenum target;
            GLuint name = fbObj->getAttachment(attachment, &target, NULL);
            if (pname == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES) {
                *params = target;
                return;
            }
            else if (pname == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES) {
                *params = name;
                return;
            }
        }
    }

    if (ctx->isDefaultFBOBound(target)) {
        SET_ERROR_IF(
            attachment == GL_DEPTH_ATTACHMENT ||
            attachment == GL_STENCIL_ATTACHMENT ||
            attachment == GL_DEPTH_STENCIL_ATTACHMENT ||
            (attachment >= GL_COLOR_ATTACHMENT0 &&
             attachment <= GL_COLOR_ATTACHMENT15), GL_INVALID_OPERATION);
        SET_ERROR_IF(pname == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, GL_INVALID_ENUM);

        if (attachment == GL_BACK)
            attachment = GL_COLOR_ATTACHMENT0;
        if (attachment == GL_DEPTH)
            attachment = GL_DEPTH_ATTACHMENT;
        if (attachment == GL_STENCIL)
            attachment = GL_STENCIL_ATTACHMENT;
    }

    ctx->dispatcher().glGetFramebufferAttachmentParameterivEXT(target,attachment,pname,params);

    if (ctx->isDefaultFBOBound(target) && *params == GL_RENDERBUFFER) {
        *params = GL_FRAMEBUFFER_DEFAULT;
    }
}

GL_API void GL_APIENTRY glGenerateMipmapOES(GLenum target) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
    SET_ERROR_IF(!GLEScmValidate::textureTargetLimited(target),GL_INVALID_ENUM);
    if (ctx->shareGroup().get()) {
        TextureData *texData = getTextureTargetData(target);
        if (texData) {
            unsigned int width = texData->width;
            unsigned int height = texData->height;
            // set error code if either the width or height is not a power of two.
            SET_ERROR_IF(width == 0 || height == 0 ||
                         (width & (width - 1)) != 0 || (height & (height - 1)) != 0,
                         GL_INVALID_OPERATION);
            texData->setMipmapLevelAtLeast(maxMipmapLevel(texData->width,
                    texData->height));
        }
    }
    ctx->dispatcher().glGenerateMipmapEXT(target);
}

GL_API void GL_APIENTRY glCurrentPaletteMatrixOES(GLuint index) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!(ctx->getCaps()->GL_ARB_MATRIX_PALETTE && ctx->getCaps()->GL_ARB_VERTEX_BLEND),GL_INVALID_OPERATION);
    ctx->dispatcher().glCurrentPaletteMatrixARB(index);
}

GL_API void GL_APIENTRY glLoadPaletteFromModelViewMatrixOES() {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!(ctx->getCaps()->GL_ARB_MATRIX_PALETTE && ctx->getCaps()->GL_ARB_VERTEX_BLEND),GL_INVALID_OPERATION);
    GLint matrix[16];
    ctx->dispatcher().glGetIntegerv(GL_MODELVIEW_MATRIX,matrix);
    ctx->dispatcher().glMatrixIndexuivARB(1,(GLuint*)matrix);

}

GL_API void GL_APIENTRY glMatrixIndexPointerOES(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!(ctx->getCaps()->GL_ARB_MATRIX_PALETTE && ctx->getCaps()->GL_ARB_VERTEX_BLEND),GL_INVALID_OPERATION);
    ctx->dispatcher().glMatrixIndexPointerARB(size,type,stride,pointer);
}

GL_API void GL_APIENTRY glWeightPointerOES(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!(ctx->getCaps()->GL_ARB_MATRIX_PALETTE && ctx->getCaps()->GL_ARB_VERTEX_BLEND),GL_INVALID_OPERATION);
    ctx->dispatcher().glWeightPointerARB(size,type,stride,pointer);

}

GL_API void GL_APIENTRY glTexGenfOES (GLenum coord, GLenum pname, GLfloat param) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texGen(coord,pname),GL_INVALID_ENUM);
    gles1usages->set_light(true);
    if (coord == GL_TEXTURE_GEN_STR_OES) {
        ctx->dispatcher().glTexGenf(GL_S,pname,param);
        ctx->dispatcher().glTexGenf(GL_T,pname,param);
        ctx->dispatcher().glTexGenf(GL_R,pname,param);
    }
    else
        ctx->dispatcher().glTexGenf(coord,pname,param);
}

GL_API void GL_APIENTRY glTexGenfvOES (GLenum coord, GLenum pname, const GLfloat *params) {
    GET_CTX()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texGen(coord,pname),GL_INVALID_ENUM);
    gles1usages->set_light(true);
    if (coord == GL_TEXTURE_GEN_STR_OES) {
        ctx->dispatcher().glTexGenfv(GL_S,pname,params);
        ctx->dispatcher().glTexGenfv(GL_T,pname,params);
        ctx->dispatcher().glTexGenfv(GL_R,pname,params);
    }
    else
        ctx->dispatcher().glTexGenfv(coord,pname,params);
}
GL_API void GL_APIENTRY glTexGeniOES (GLenum coord, GLenum pname, GLint param) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texGen(coord,pname),GL_INVALID_ENUM);
    gles1usages->set_light(true);
    ctx->texGeni(coord, pname, param);
}
GL_API void GL_APIENTRY glTexGenivOES (GLenum coord, GLenum pname, const GLint *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texGen(coord,pname),GL_INVALID_ENUM);
    gles1usages->set_light(true);
    ctx->texGeniv(coord, pname, params);
}
GL_API void GL_APIENTRY glTexGenxOES (GLenum coord, GLenum pname, GLfixed param) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texGen(coord,pname),GL_INVALID_ENUM);
    gles1usages->set_light(true);
    ctx->texGenf(coord, pname, X2F(param));
}
GL_API void GL_APIENTRY glTexGenxvOES (GLenum coord, GLenum pname, const GLfixed *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    SET_ERROR_IF(!GLEScmValidate::texGen(coord,pname),GL_INVALID_ENUM);
    gles1usages->set_light(true);
    GLfloat tmpParams[1];
    tmpParams[0] = X2F(params[0]);
    ctx->texGenfv(coord, pname, tmpParams);
}

GL_API void GL_APIENTRY glGetTexGenfvOES (GLenum coord, GLenum pname, GLfloat *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->getTexGenfv(coord, pname, params);
}
GL_API void GL_APIENTRY glGetTexGenivOES (GLenum coord, GLenum pname, GLint *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    ctx->getTexGeniv(coord, pname, params);
}

GL_API void GL_APIENTRY glGetTexGenxvOES (GLenum coord, GLenum pname, GLfixed *params) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    GLfloat tmpParams[1];
    ctx->getTexGenfv(coord, pname, tmpParams);
    params[0] = F2X(tmpParams[0]);
}


template <class T, GLenum TypeName>
void glDrawTexOES (T x, T y, T z, T width, T height) {
    GET_CTX_CM()
    GLES_CM_TRACE()

    SET_ERROR_IF((width<=0 || height<=0),GL_INVALID_VALUE);

    ctx->drawValidate();
    ctx->drawTexOES((float)x, (float)y, (float)z, (float)width, (float)height);
}

GL_API void GL_APIENTRY glDrawTexsOES (GLshort x, GLshort y, GLshort z, GLshort width, GLshort height) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    glDrawTexOES<GLshort,GL_SHORT>(x,y,z,width,height);
}

GL_API void GL_APIENTRY glDrawTexiOES (GLint x, GLint y, GLint z, GLint width, GLint height) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    glDrawTexOES<GLint,GL_INT>(x,y,z,width,height);
}

GL_API void GL_APIENTRY glDrawTexfOES (GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    glDrawTexOES<GLfloat,GL_FLOAT>(x,y,z,width,height);
}

GL_API void GL_APIENTRY glDrawTexxOES (GLfixed x, GLfixed y, GLfixed z, GLfixed width, GLfixed height) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    glDrawTexOES<GLfloat,GL_FLOAT>(X2F(x),X2F(y),X2F(z),X2F(width),X2F(height));
}

GL_API void GL_APIENTRY glDrawTexsvOES (const GLshort * coords) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    glDrawTexOES<GLshort,GL_SHORT>(coords[0],coords[1],coords[2],coords[3],coords[4]);
}

GL_API void GL_APIENTRY glDrawTexivOES (const GLint * coords) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    glDrawTexOES<GLint,GL_INT>(coords[0],coords[1],coords[2],coords[3],coords[4]);
}

GL_API void GL_APIENTRY glDrawTexfvOES (const GLfloat * coords) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    glDrawTexOES<GLfloat,GL_FLOAT>(coords[0],coords[1],coords[2],coords[3],coords[4]);
}

GL_API void GL_APIENTRY glDrawTexxvOES (const GLfixed * coords) {
    GET_CTX_CM()
    GLES_CM_TRACE()
    glDrawTexOES<GLfloat,GL_FLOAT>(X2F(coords[0]),X2F(coords[1]),X2F(coords[2]),X2F(coords[3]),X2F(coords[4]));
}
