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

#include <stdio.h>
#include <GLcommon/gldefs.h>
#include <GLcommon/TranslatorIfaces.h>
#include <GLcommon/GLconversion_macros.h>
#include "GLEScmRouting.h"
#include "GLEScmValidate.h"
#include "GLEScmContext.h"
#include "es1xAPI.h"

#include <GLES/gl.h>
#include <GLES/glext.h>

// #include <GLcommon/FramebufferData.h>
// #include <cmath>
// #include <map>

// ---------------------------------------------------------------------------
extern "C" {
// ---------------------------------------------------------------------------

//declaration
static void initGLESx();
static void initContext(GLEScontext* ctx,ShareGroupPtr grp);
static void deleteGLESContext(GLEScontext* ctx);
static void setShareGroup(GLEScontext* ctx,ShareGroupPtr grp);
static GLEScontext* createGLESContext();
static __translatorMustCastToProperFunctionPointerType getProcAddress(const char* procName);

}

/************************************** GLES EXTENSIONS *********************************************************/
//extentions descriptor
typedef std::map<std::string, __translatorMustCastToProperFunctionPointerType> ProcTableMap;
ProcTableMap *s_glesExtensions = NULL;
/****************************************************************************************************************/

static EGLiface*  s_eglIface = NULL;
static GLESiface  s_glesIface = {
  .initGLESx         = initGLESx,
  .createGLESContext = createGLESContext,
  .initContext       = initContext,
  .deleteGLESContext = deleteGLESContext,
  .flush             = (FUNCPTR)glFlush,
  .finish            = (FUNCPTR)glFinish,
  .setShareGroup     = setShareGroup,
  .getProcAddress    = getProcAddress
};

#include <GLcommon/GLESmacros.h>

extern "C" {

static void initGLESx() {
  fprintf(stdout, "Init GLESv1 by loading routing table to GLES_V2\n");
  init_gles_v1tov2_routing();
  return;
}

static void initContext(GLEScontext* ctx,ShareGroupPtr grp) {
  if (!ctx->isInitialized()) {
    ctx->setShareGroup(grp);
    ctx->init();
    glBindTexture(GL_TEXTURE_2D,0);
    glBindTexture(GL_TEXTURE_CUBE_MAP_OES,0);
  }
}

static GLEScontext* createGLESContext() {
  return new GLEScmContext();
}

static void deleteGLESContext(GLEScontext* ctx) {
  if(ctx) delete ctx;
}

static void setShareGroup(GLEScontext* ctx,ShareGroupPtr grp) {
  if(ctx) {
    ctx->setShareGroup(grp);
  }
}

static __translatorMustCastToProperFunctionPointerType getProcAddress(const char* procName) {
  GET_CTX_RET(NULL);
  ctx->getGlobalLock();
  static bool proc_table_initialized = false;
  if (!proc_table_initialized) {
    proc_table_initialized = true;
    if (!s_glesExtensions)
      s_glesExtensions = new ProcTableMap();
    else
      s_glesExtensions->clear();
    (*s_glesExtensions)["glEGLImageTargetTexture2DOES"] = (__translatorMustCastToProperFunctionPointerType) glEGLImageTargetTexture2DOES;
    (*s_glesExtensions)["glEGLImageTargetRenderbufferStorageOES"]=(__translatorMustCastToProperFunctionPointerType)glEGLImageTargetRenderbufferStorageOES;
    if (ctx->getCaps()->GL_ARB_MATRIX_PALETTE && ctx->getCaps()->GL_ARB_VERTEX_BLEND) {
      (*s_glesExtensions)["glFrustumfOES"] = (__translatorMustCastToProperFunctionPointerType)glFrustumf;
    }
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
      (*s_glesExtensions)["glFramebufferRenderbufferOES"] = (__translatorMustCastToProperFunctionPointerType)glFramebufferRenderbufferOES;
      (*s_glesExtensions)["glFramebufferTexture2DOES"] = (__translatorMustCastToProperFunctionPointerType)glFramebufferTexture2DOES;
      (*s_glesExtensions)["glGetFramebufferAttachmentParameterivOES"] = (__translatorMustCastToProperFunctionPointerType)glGetFramebufferAttachmentParameterivOES;
      (*s_glesExtensions)["glGenerateMipmapOES"] = (__translatorMustCastToProperFunctionPointerType)glGenerateMipmapOES;
    }
  }
  __translatorMustCastToProperFunctionPointerType ret=NULL;
  ProcTableMap::iterator val = s_glesExtensions->find(procName);
  if (val!=s_glesExtensions->end())
    ret = val->second;
  ctx->releaseGlobalLock();

  return ret;
}

GL_API GLESiface* __translator_getIfaces(EGLiface* eglIface){
  s_eglIface = eglIface;
  return & s_glesIface;
}

// ---------------------------------------------------------------------------
} // end extern{C}
// ---------------------------------------------------------------------------

// static ObjectLocalName TextureLocalName(GLenum target, unsigned int tex) {
//   GET_CTX_RET(0);
//   return (tex!=0? tex : ctx->getDefaultTextureName(target));
// }

// static TextureData* getTextureData(ObjectLocalName tex){
//   GET_CTX_RET(NULL);

//   if(!ctx->shareGroup()->isObject(TEXTURE,tex))
//   {
//     return NULL;
//   }

//   TextureData *texData = NULL;
//   ObjectDataPtr objData = ctx->shareGroup()->getObjectData(TEXTURE,tex);
//   if(!objData.Ptr()){
//     texData = new TextureData();
//     ctx->shareGroup()->setObjectData(TEXTURE, tex, ObjectDataPtr(texData));
//   } else {
//     texData = (TextureData*)objData.Ptr();
//   }
//   return texData;
// }

// static TextureData* getTextureTargetData(GLenum target){
//   GET_CTX_RET(NULL);
//   unsigned int tex = ctx->getBindedTexture(target);
//   return getTextureData(TextureLocalName(target,tex));
// }

//-----------------------------------------------------------------------------
// GL_API calls :
//-----------------------------------------------------------------------------

GL_API GLboolean GL_APIENTRY glIsBuffer(GLuint buffer) {
  GET_CTX_CM_RET(GL_FALSE)
      fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  return es1xIsBuffer(ctx->getES1xContext(), buffer);
}

GL_API GLboolean GL_APIENTRY  glIsEnabled( GLenum cap) {
  GET_CTX_CM_RET(GL_FALSE);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  return es1xIsEnabled(ctx->getES1xContext(), cap);
}


GL_API GLboolean GL_APIENTRY  glIsTexture( GLuint texture) {
  GET_CTX_CM_RET(GL_FALSE);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  return es1xIsTexture(ctx->getES1xContext(), texture);
}

GL_API GLenum GL_APIENTRY  glGetError(void) {
  GET_CTX_CM_RET(GL_NO_ERROR);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  return es1xGetError(ctx->getES1xContext());
}

GL_API const GLubyte * GL_APIENTRY  glGetString( GLenum name) {
  GET_CTX_RET(NULL);
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

GL_API void GL_APIENTRY  glActiveTexture(GLenum texture) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::textureEnum(texture,ctx->getMaxTexUnits()),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xActiveTexture(ctx->getES1xContext(), texture);
  return;
}

GL_API void GL_APIENTRY  glAlphaFunc(GLenum func, GLclampf ref) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::alphaFunc(func),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xAlphaFunc(ctx->getES1xContext(), func, ref);
  return;
}

GL_API void GL_APIENTRY  glAlphaFuncx(GLenum func, GLclampx ref) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  SET_ERROR_IF(!GLEScmValidate::alphaFunc(func),GL_INVALID_ENUM);
  es1xAlphaFuncx(ctx->getES1xContext(), func, ref);
  return;
}

GL_API void GL_APIENTRY  glBindBuffer(GLenum target, GLuint buffer) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::bufferTarget(target),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xBindBuffer(ctx->getES1xContext(), target, buffer);
  return;
}

GL_API void GL_APIENTRY  glBindTexture(GLenum target, GLuint texture) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::textureTarget(target),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xBindTexture(ctx->getES1xContext(), target, texture);
  return;
}

GL_API void GL_APIENTRY  glBlendFunc( GLenum sfactor, GLenum dfactor) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::blendSrc(sfactor) || !GLEScmValidate::blendDst(dfactor),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xBlendFunc(ctx->getES1xContext(), sfactor, dfactor);
  return;
}

GL_API void GL_APIENTRY  glBufferData( GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::bufferTarget(target),GL_INVALID_ENUM);
  SET_ERROR_IF(!ctx->isBindedBuffer(target),GL_INVALID_OPERATION);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xBufferData(ctx->getES1xContext(), target, size, data, usage);
  return;
}

GL_API void GL_APIENTRY  glBufferSubData( GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data) {
  GET_CTX_CM();
  SET_ERROR_IF(!ctx->isBindedBuffer(target),GL_INVALID_OPERATION);
  SET_ERROR_IF(!GLEScmValidate::bufferTarget(target),GL_INVALID_ENUM);
  SET_ERROR_IF(!ctx->setBufferSubData(target,offset,size,data),GL_INVALID_VALUE);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xBufferSubData(ctx->getES1xContext(), target, offset, size, data);
  return;
}

GL_API void GL_APIENTRY  glClear( GLbitfield mask) {
  GET_CTX_CM();
  ctx->drawValidate();

  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xClear(ctx->getES1xContext(), mask);
  return;
}

GL_API void GL_APIENTRY  glClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xClearColor(ctx->getES1xContext(), red, green, blue, alpha);
  return;
}

GL_API void GL_APIENTRY  glClearColorx( GLclampx red, GLclampx green, GLclampx blue, GLclampx alpha) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xClearColorx(ctx->getES1xContext(), red, green, blue, alpha);
  return;
}


GL_API void GL_APIENTRY  glClearDepthf( GLclampf depth) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p--> ", __func__, ctx);
  es1xClearDepthf(ctx->getES1xContext(), depth);
  return;
}

GL_API void GL_APIENTRY  glClearDepthx( GLclampx depth) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p--> ", __func__, ctx);
  es1xClearDepthx(ctx->getES1xContext(), depth);
  return;
}

GL_API void GL_APIENTRY  glClearStencil(GLint s) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p--> ", __func__, ctx);
  es1xClearStencil(ctx->getES1xContext(), s);
  return;
}

GL_API void GL_APIENTRY  glClientActiveTexture(GLenum texture) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::textureEnum(texture,ctx->getMaxTexUnits()),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p--> ", __func__, ctx);
  es1xClientActiveTexture(ctx->getES1xContext(), texture);
  return;
}

GL_API void GL_APIENTRY  glClipPlanef(GLenum plane, const GLfloat *equation) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p--> ", __func__, ctx);
  es1xClipPlanef(ctx->getES1xContext(), plane, equation);
  return;
}

GL_API void GL_APIENTRY  glClipPlanex(GLenum plane, const GLfixed *equation) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p--> ", __func__, ctx);
  es1xClipPlanex(ctx->getES1xContext(), plane, equation);
  return;
}

GL_API void GL_APIENTRY  glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p--> ", __func__, ctx);
  es1xColor4f(ctx->getES1xContext(), red, green, blue, alpha);
  return;
}

GL_API void GL_APIENTRY  glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p--> ", __func__, ctx);
  es1xColor4ub(ctx->getES1xContext(), red, green, blue, alpha);
  return;
}

GL_API void GL_APIENTRY  glColor4x(GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p--> ", __func__, ctx);
  es1xColor4x(ctx->getES1xContext(), red, green, blue, alpha);
  return;
}

GL_API void GL_APIENTRY  glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p--> ", __func__, ctx);
  es1xColorMask(ctx->getES1xContext(), red, green, blue, alpha);
  return;
}

GL_API void GL_APIENTRY  glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::colorPointerParams(size,stride),GL_INVALID_VALUE);
  SET_ERROR_IF(!GLEScmValidate::colorPointerType(type),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p--> ", __func__, ctx);
  es1xColorPointer(ctx->getES1xContext(), size, type, stride, pointer);
  return;
}

GL_API void GL_APIENTRY  glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat,
                                                GLsizei width, GLsizei height, GLint border,
                                                GLsizei imageSize, const GLvoid *data) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::textureTargetEx(target),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xCompressedTexImage2D(ctx->getES1xContext(), target, level, internalformat, width, height, border, imageSize, data);
  return;
}

GL_API void GL_APIENTRY  glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data) {
  GET_CTX_CM();
  SET_ERROR_IF(!(GLEScmValidate::texCompImgFrmt(format) && GLEScmValidate::textureTargetEx(target)),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xCompressedTexSubImage2D(ctx->getES1xContext(), target, level, xoffset, yoffset, width, height, format, imageSize, data);
  return;
}

GL_API void GL_APIENTRY  glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
  GET_CTX_CM();
  SET_ERROR_IF(!(GLEScmValidate::pixelFrmt(ctx,internalformat) && GLEScmValidate::textureTargetEx(target)),GL_INVALID_ENUM);
  SET_ERROR_IF(border != 0,GL_INVALID_VALUE);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xCopyTexImage2D(ctx->getES1xContext(), target, level, internalformat, x, y, width, height, border);
  return;
}

GL_API void GL_APIENTRY  glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::textureTargetEx(target),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xCopyTexSubImage2D(ctx->getES1xContext(), target, level, xoffset, yoffset, x, y, width, height);
  return;
}

GL_API void GL_APIENTRY  glCullFace(GLenum mode) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xCullFace(ctx->getES1xContext(), mode);
  return;
}

GL_API void GL_APIENTRY  glDeleteBuffers(GLsizei n, const GLuint *buffers) {
  GET_CTX_CM();
  SET_ERROR_IF(n<0,GL_INVALID_VALUE);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xDeleteBuffers(ctx->getES1xContext(), n, buffers);
  return;
}

GL_API void GL_APIENTRY  glDeleteTextures(GLsizei n, const GLuint *textures) {
  GET_CTX_CM();
  SET_ERROR_IF(n<0,GL_INVALID_VALUE);
  fprintf(stdout, "v1: %s ctx=%p-->", __func__, ctx);
  es1xDeleteTextures(ctx->getES1xContext(), n, textures);
  return;
}

GL_API void GL_APIENTRY  glDepthFunc(GLenum func) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xDepthFunc(ctx->getES1xContext(), func);
  return;
}

GL_API void GL_APIENTRY  glDepthMask(GLboolean flag) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xDepthMask(ctx->getES1xContext(), flag);
  return;
}

GL_API void GL_APIENTRY  glDepthRangef(GLclampf zNear, GLclampf zFar) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xDepthRangef(ctx->getES1xContext(), zNear, zFar);
  return;
}

GL_API void GL_APIENTRY  glDepthRangex(GLclampx zNear, GLclampx zFar) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xDepthRangex(ctx->getES1xContext(), zNear, zFar);
  return;
}

GL_API void GL_APIENTRY  glDisable(GLenum cap) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xDisable(ctx->getES1xContext(), cap);
  return;
}

GL_API void GL_APIENTRY  glDisableClientState(GLenum array) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::supportedArrays(array),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xDisableClientState(ctx->getES1xContext(), array);
  return;
}

GL_API void GL_APIENTRY  glDrawArrays(GLenum mode, GLint first, GLsizei count) {
  GET_CTX_CM();
  SET_ERROR_IF(count < 0,GL_INVALID_VALUE);
  SET_ERROR_IF(!GLEScmValidate::drawMode(mode),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xDrawArrays(ctx->getES1xContext(), mode, first, count);
  return;
}

GL_API void GL_APIENTRY  glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *elementsIndices) {
  GET_CTX_CM();
  SET_ERROR_IF(count < 0,GL_INVALID_VALUE);
  SET_ERROR_IF((!GLEScmValidate::drawMode(mode) || !GLEScmValidate::drawType(type)),GL_INVALID_ENUM);

  // // TODO: consider leaving all validation to GLES_V2
  // if(!ctx->isArrEnabled(GL_VERTEX_ARRAY)) {
  //   fprintf(stdout, "v1: %s ctx=%p ! isArrEnabled ", __func__, ctx);
  //   return;
  // }
  ctx->drawValidate();

  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xDrawElements(ctx->getES1xContext(), mode, count, type, elementsIndices);
  return;
}

  GL_API void GL_APIENTRY  glEnable(GLenum cap) {
    GET_CTX_CM();
    fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
    es1xEnable(ctx->getES1xContext(), cap);
    return;
  }

GL_API void GL_APIENTRY  glEnableClientState(GLenum array) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::supportedArrays(array),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xEnableClientState(ctx->getES1xContext(), array);
  return;
}

GL_API void GL_APIENTRY  glFinish( void) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xFinish(ctx->getES1xContext());
  return;
}

GL_API void GL_APIENTRY  glFlush( void) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xFlush(ctx->getES1xContext());
  return;
}

GL_API void GL_APIENTRY  glFogf(GLenum pname, GLfloat param) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xFogf(ctx->getES1xContext(), pname, param);
  return;
}

GL_API void GL_APIENTRY  glFogfv(GLenum pname, const GLfloat *params) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xFogfv(ctx->getES1xContext(), pname, params);
  return;
}

GL_API void GL_APIENTRY  glFogx(GLenum pname, GLfixed param) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xFogx(ctx->getES1xContext(), pname, param);
  return;
}

GL_API void GL_APIENTRY  glFogxv(GLenum pname, const GLfixed *params) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xFogxv(ctx->getES1xContext(), pname, params);
  return;
}

GL_API void GL_APIENTRY  glFrontFace(GLenum mode) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xFrontFace(ctx->getES1xContext(), mode);
  return;
}

GL_API void GL_APIENTRY  glFrustumf( GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xFrustumf(ctx->getES1xContext(), left,right,bottom,top,zNear,zFar);
  return;
}

GL_API void GL_APIENTRY  glFrustumx( GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xFrustumx(ctx->getES1xContext(), left,right,bottom,top,zNear,zFar);
  return;
}

GL_API void GL_APIENTRY  glGenBuffers(GLsizei n, GLuint *buffers) {
  GET_CTX_CM();
  SET_ERROR_IF(n<0,GL_INVALID_VALUE);
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xGenBuffers(ctx->getES1xContext(), n, buffers);
  return;
}

GL_API void GL_APIENTRY  glGenTextures(GLsizei n, GLuint *textures) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xGenTextures(ctx->getES1xContext(), n, textures);
  return;
}

GL_API void GL_APIENTRY  glGetBooleanv(GLenum pname, GLboolean *params) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xGetBooleanv(ctx->getES1xContext(), pname, params);
  return;
}

GL_API void GL_APIENTRY  glGetBufferParameteriv(GLenum target, GLenum pname, GLint *params) {
  GET_CTX_CM();
  SET_ERROR_IF(!(GLEScmValidate::bufferTarget(target) && GLEScmValidate::bufferParam(pname)),GL_INVALID_ENUM);
  SET_ERROR_IF(!ctx->isBindedBuffer(target),GL_INVALID_OPERATION);
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xGetBufferParameteriv(ctx->getES1xContext(), target, pname, params);

  return;
}

GL_API void GL_APIENTRY  glGetClipPlanef(GLenum pname, GLfloat eqn[4]) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xGetClipPlanef(ctx->getES1xContext(), pname, eqn);
  return;
}

GL_API void GL_APIENTRY  glGetClipPlanex(GLenum pname, GLfixed eqn[4]) {
  GET_CTX_CM()
      fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xGetClipPlanex(ctx->getES1xContext(), pname, eqn);
  return;
}

GL_API void GL_APIENTRY  glGetFixedv(GLenum pname, GLfixed *params) {
  GET_CTX_CM()
      fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xGetFixedv(ctx->getES1xContext(), pname, params);
  return;
}

GL_API void GL_APIENTRY  glGetFloatv(GLenum pname, GLfloat *params) {
  GET_CTX_CM()
      fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xGetFloatv(ctx->getES1xContext(), pname, params);
  return;
}

GL_API void GL_APIENTRY  glGetIntegerv(GLenum pname, GLint *params) {
  GET_CTX_CM()
      fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xGetIntegerv(ctx->getES1xContext(), pname, params);
  return;
}

GL_API void GL_APIENTRY  glGetLightfv(GLenum light, GLenum pname, GLfloat *params) {
  GET_CTX_CM()
      fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xGetLightfv(ctx->getES1xContext(), light, pname, params);
  return;
}

GL_API void GL_APIENTRY  glGetLightxv(GLenum light, GLenum pname, GLfixed *params) {
  GET_CTX_CM()
      fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xGetLightxv(ctx->getES1xContext(), light, pname, params);
  return;
}

GL_API void GL_APIENTRY  glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params) {
  GET_CTX_CM()
      fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xGetMaterialfv(ctx->getES1xContext(), face, pname, params);
  return;
}

GL_API void GL_APIENTRY  glGetMaterialxv(GLenum face, GLenum pname, GLfixed *params) {
  GET_CTX_CM()
      fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xGetMaterialxv(ctx->getES1xContext(), face, pname, params);
  return;
}

GL_API void GL_APIENTRY  glGetPointerv(GLenum pname, void **params) {
  GET_CTX_CM()
      fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xGetPointerv(ctx->getES1xContext(), pname, params);
  return;
}

GL_API void GL_APIENTRY  glGetTexEnvfv(GLenum env, GLenum pname, GLfloat *params) {
  GET_CTX_CM()
      fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xGetTexEnvfv(ctx->getES1xContext(), env, pname, params);
  return;
}

GL_API void GL_APIENTRY  glGetTexEnviv(GLenum env, GLenum pname, GLint *params) {
  GET_CTX_CM()
      fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xGetTexEnviv(ctx->getES1xContext(), env, pname, params);
  return;
}

GL_API void GL_APIENTRY  glGetTexEnvxv(GLenum env, GLenum pname, GLfixed *params) {
  GET_CTX_CM()
      fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xGetTexEnvxv(ctx->getES1xContext(), env, pname, params);
  return;
}

GL_API void GL_APIENTRY  glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params) {
  GET_CTX_CM()
      fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xGetTexParameterfv(ctx->getES1xContext(), target, pname, params);
  return;
}

GL_API void GL_APIENTRY  glGetTexParameteriv(GLenum target, GLenum pname, GLint *params) {
  GET_CTX_CM()
      fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xGetTexParameteriv(ctx->getES1xContext(), target, pname, params);
  return;
}

GL_API void GL_APIENTRY  glGetTexParameterxv(GLenum target, GLenum pname, GLfixed *params) {
  GET_CTX_CM()
      fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xGetTexParameterxv(ctx->getES1xContext(), target, pname, params);
  return;
}

GL_API void GL_APIENTRY  glHint(GLenum target, GLenum mode) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::hintTargetMode(target,mode),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xHint(ctx->getES1xContext(), target, mode);
  return;
}

GL_API void GL_APIENTRY  glLightModelf(GLenum pname, GLfloat param) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xLightModelf(ctx->getES1xContext(), pname, param);
  return;
}

GL_API void GL_APIENTRY  glLightModelfv(GLenum pname, const GLfloat *params) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xLightModelfv(ctx->getES1xContext(), pname, params);
  return;
}

GL_API void GL_APIENTRY  glLightModelx(GLenum pname, GLfixed param) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xLightModelx(ctx->getES1xContext(), pname, param);
  return;
}

GL_API void GL_APIENTRY  glLightModelxv(GLenum pname, const GLfixed *params) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xLightModelxv(ctx->getES1xContext(), pname, params);
  return;
}

GL_API void GL_APIENTRY  glLightf(GLenum light, GLenum pname, GLfloat param) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xLightf(ctx->getES1xContext(), light, pname, param);
  return;
}

GL_API void GL_APIENTRY  glLightfv(GLenum light, GLenum pname, const GLfloat *params) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xLightfv(ctx->getES1xContext(), light, pname, params);
  return;
}

GL_API void GL_APIENTRY  glLightx(GLenum light, GLenum pname, GLfixed param) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xLightx(ctx->getES1xContext(), light, pname, param);
  return;
}

GL_API void GL_APIENTRY  glLightxv(GLenum light, GLenum pname, const GLfixed *params) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xLightxv(ctx->getES1xContext(), light, pname, params);
  return;
}

GL_API void GL_APIENTRY  glLineWidth(GLfloat width) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xLineWidth(ctx->getES1xContext(), width);
  return;
}

GL_API void GL_APIENTRY  glLineWidthx(GLfixed width) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xLineWidthx(ctx->getES1xContext(), width);
  return;
}

GL_API void GL_APIENTRY  glLoadIdentity( void) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xLoadIdentity(ctx->getES1xContext());
  return;
}

GL_API void GL_APIENTRY  glLoadMatrixf( const GLfloat *m) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xLoadMatrixf(ctx->getES1xContext(), m);
  return;
}

GL_API void GL_APIENTRY  glLoadMatrixx( const GLfixed *m) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xLoadMatrixx(ctx->getES1xContext(), m);
  return;
}

GL_API void GL_APIENTRY  glLogicOp(GLenum opcode) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xLogicOp(ctx->getES1xContext(), opcode);
  return;
}

GL_API void GL_APIENTRY  glMaterialf(GLenum face, GLenum pname, GLfloat param) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xMaterialf(ctx->getES1xContext(), face, pname, param);
  return;
}

GL_API void GL_APIENTRY  glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xMaterialfv(ctx->getES1xContext(), face, pname, params);
  return;
}

GL_API void GL_APIENTRY  glMaterialx(GLenum face, GLenum pname, GLfixed param) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xMaterialx(ctx->getES1xContext(), face, pname, param);
  return;
}

GL_API void GL_APIENTRY  glMaterialxv(GLenum face, GLenum pname, const GLfixed *params) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xMaterialxv(ctx->getES1xContext(), face, pname, params);
  return;
}

GL_API void GL_APIENTRY  glMatrixMode(GLenum mode) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xMatrixMode(ctx->getES1xContext(), mode);
  return;
}

GL_API void GL_APIENTRY  glMultMatrixf( const GLfloat *m) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xMultMatrixf(ctx->getES1xContext(), m);
  return;
}

GL_API void GL_APIENTRY  glMultMatrixx( const GLfixed *m) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xMultMatrixx(ctx->getES1xContext(), m);
  return;
}

GL_API void GL_APIENTRY  glMultiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::textureEnum(target,ctx->getMaxTexUnits()),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xMultiTexCoord4f(ctx->getES1xContext(), target, s, t, r, q);
  return;
}

GL_API void GL_APIENTRY  glMultiTexCoord4x(GLenum target, GLfixed s, GLfixed t, GLfixed r, GLfixed q) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::textureEnum(target,ctx->getMaxTexUnits()),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xMultiTexCoord4x(ctx->getES1xContext(), target, s, t, r, q);
  return;
}

GL_API void GL_APIENTRY  glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xNormal3f(ctx->getES1xContext(), nx, ny, nz);
  return;
}

GL_API void GL_APIENTRY  glNormal3x(GLfixed nx, GLfixed ny, GLfixed nz) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xNormal3x(ctx->getES1xContext(), nx, ny, nz);
  return;
}

GL_API void GL_APIENTRY  glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  SET_ERROR_IF(stride < 0,GL_INVALID_VALUE);
  SET_ERROR_IF(!GLEScmValidate::normalPointerType(type),GL_INVALID_ENUM);
  es1xNormalPointer(ctx->getES1xContext(), type, stride, pointer);
  return;
}

GL_API void GL_APIENTRY  glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xOrthof(ctx->getES1xContext(), left, right, bottom, top, zNear, zFar);
  return;
}

GL_API void GL_APIENTRY  glOrthox(GLfixed left, GLfixed right, GLfixed bottom, GLfixed top, GLfixed zNear, GLfixed zFar) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xOrthox(ctx->getES1xContext(), left, right, bottom, top, zNear, zFar);
  return;

}

GL_API void GL_APIENTRY  glPixelStorei( GLenum pname, GLint param) {
  GET_CTX_CM();
  SET_ERROR_IF(!(pname == GL_PACK_ALIGNMENT || pname == GL_UNPACK_ALIGNMENT),GL_INVALID_ENUM);
  SET_ERROR_IF(!((param==1)||(param==2)||(param==4)||(param==8)), GL_INVALID_VALUE);
  es1xPixelStorei(ctx->getES1xContext(), pname, param);
  return;
}

GL_API void GL_APIENTRY  glPointParameterf(GLenum pname, GLfloat param) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xPointParameterf(ctx->getES1xContext(), pname, param);
  return;
}

GL_API void GL_APIENTRY  glPointParameterfv(GLenum pname, const GLfloat *params) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xPointParameterfv(ctx->getES1xContext(), pname, params);
  return;
}

GL_API void GL_APIENTRY  glPointParameterx(GLenum pname, GLfixed param) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xPointParameterx(ctx->getES1xContext(), pname, param);
  return;
}

GL_API void GL_APIENTRY  glPointParameterxv(GLenum pname, const GLfixed *params) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xPointParameterxv(ctx->getES1xContext(), pname, params);
  return;
}

GL_API void GL_APIENTRY  glPointSize(GLfloat size) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xPointSize(ctx->getES1xContext(), size);
  return;
}

GL_API void GL_APIENTRY  glPointSizePointerOES(GLenum type, GLsizei stride, const GLvoid *pointer) {
  GET_CTX_CM();;
  SET_ERROR_IF(stride < 0,GL_INVALID_VALUE);
  SET_ERROR_IF(!GLEScmValidate::pointPointerType(type),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xPointSizePointerOES(ctx->getES1xContext(), type, stride, pointer);
  return;
}

GL_API void GL_APIENTRY  glPointSizex(GLfixed size) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xPointSizex(ctx->getES1xContext(), size);
  return;
}

GL_API void GL_APIENTRY  glPolygonOffset(GLfloat factor, GLfloat units) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xPolygonOffset(ctx->getES1xContext(), factor, units);
  return;
}

GL_API void GL_APIENTRY  glPolygonOffsetx(GLfixed factor, GLfixed units) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xPolygonOffsetx(ctx->getES1xContext(), factor, units);
  return;
}

GL_API void GL_APIENTRY  glPopMatrix(void) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xPopMatrix(ctx->getES1xContext());
  return;
}

GL_API void GL_APIENTRY  glPushMatrix(void) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xPushMatrix(ctx->getES1xContext());
  return;
}

GL_API void GL_APIENTRY  glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {
  GET_CTX_CM();
  SET_ERROR_IF(!(GLEScmValidate::pixelFrmt(ctx,format) && GLEScmValidate::pixelType(ctx,type)),GL_INVALID_ENUM);
  SET_ERROR_IF(!(GLEScmValidate::pixelOp(format,type)),GL_INVALID_OPERATION);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xReadPixels(ctx->getES1xContext(), x, y, width, height, format, type, pixels);
  return;
}

GL_API void GL_APIENTRY  glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xRotatef(ctx->getES1xContext(), angle, x, y, z);
  return;
}

GL_API void GL_APIENTRY  glRotatex(GLfixed angle, GLfixed x, GLfixed y, GLfixed z) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xRotatex(ctx->getES1xContext(), angle, x, y, z);
  return;
}

GL_API void GL_APIENTRY  glSampleCoverage(GLclampf value, GLboolean invert) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xSampleCoverage(ctx->getES1xContext(), value, invert);
  return;
}

GL_API void GL_APIENTRY  glSampleCoveragex(GLclampx value, GLboolean invert) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xSampleCoveragex(ctx->getES1xContext(), value, invert);
  return;
}

GL_API void GL_APIENTRY  glScalef(GLfloat x, GLfloat y, GLfloat z) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xScalef(ctx->getES1xContext(), x, y, z);
  return;
}

GL_API void GL_APIENTRY  glScalex(GLfixed x, GLfixed y, GLfixed z) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xScalex(ctx->getES1xContext(), x, y, z);
  return;
}

GL_API void GL_APIENTRY  glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xScissor(ctx->getES1xContext(), x, y, width, height);
  return;
}

GL_API void GL_APIENTRY  glShadeModel(GLenum mode) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xShadeModel(ctx->getES1xContext(), mode);
  return;
}

GL_API void GL_APIENTRY  glStencilFunc(GLenum func, GLint ref, GLuint mask) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xStencilFunc(ctx->getES1xContext(), func, ref, mask);
  return;
}

GL_API void GL_APIENTRY  glStencilMask(GLuint mask) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xStencilMask(ctx->getES1xContext(), mask);
  return;
}

GL_API void GL_APIENTRY  glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
  GET_CTX_CM();
  SET_ERROR_IF(!(GLEScmValidate::stencilOp(fail) && GLEScmValidate::stencilOp(zfail) && GLEScmValidate::stencilOp(zpass)),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xStencilOp(ctx->getES1xContext(), fail, zfail, zpass);
  return;
}

GL_API void GL_APIENTRY  glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texCoordPointerParams(size,stride),GL_INVALID_VALUE);
  SET_ERROR_IF(!GLEScmValidate::texCoordPointerType(type),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xTexCoordPointer(ctx->getES1xContext(), size, type, stride, pointer);
  return;
}

GL_API void GL_APIENTRY  glTexEnvf(GLenum target, GLenum pname, GLfloat param) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texEnv(target,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xTexEnvf(ctx->getES1xContext(), target, pname, param);
  return;
}

GL_API void GL_APIENTRY  glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texEnv(target,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xTexEnvfv(ctx->getES1xContext(), target, pname, params);
  return;
}

GL_API void GL_APIENTRY  glTexEnvi(GLenum target, GLenum pname, GLint param) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texEnv(target,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xTexEnvi(ctx->getES1xContext(), target, pname, param);
  return;
}

GL_API void GL_APIENTRY  glTexEnviv(GLenum target, GLenum pname, const GLint *params) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texEnv(target,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xTexEnviv(ctx->getES1xContext(), target, pname, params);
  return;
}

GL_API void GL_APIENTRY  glTexEnvx(GLenum target, GLenum pname, GLfixed param) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texEnv(target,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xTexEnvx(ctx->getES1xContext(), target, pname, param);
  return;
}

GL_API void GL_APIENTRY  glTexEnvxv(GLenum target, GLenum pname, const GLfixed *params) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texEnv(target,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xTexEnvxv(ctx->getES1xContext(), target, pname, params);
  return;
}

GL_API void GL_APIENTRY  glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
  GET_CTX_CM();
  SET_ERROR_IF(!(GLEScmValidate::textureTargetEx(target) &&
                 GLEScmValidate::pixelFrmt(ctx,internalformat) &&
                 GLEScmValidate::pixelFrmt(ctx,format) &&
                 GLEScmValidate::pixelType(ctx,type)),GL_INVALID_ENUM);

  SET_ERROR_IF(!(GLEScmValidate::pixelOp(format,type) && internalformat == ((GLint)format)),GL_INVALID_OPERATION);

  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xTexImage2D(ctx->getES1xContext(), target, level, internalformat, width, height, border, format, type, pixels);
  return;
}

GL_API void GL_APIENTRY  glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texParams(target,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xTexParameterf(ctx->getES1xContext(), target, pname, param);
  return;
}

GL_API void GL_APIENTRY  glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texParams(target,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xTexParameterfv(ctx->getES1xContext(), target, pname, params);
  return;
}

GL_API void GL_APIENTRY  glTexParameteri(GLenum target, GLenum pname, GLint param) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texParams(target,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xTexParameteri(ctx->getES1xContext(), target, pname, param);
  return;
}

GL_API void GL_APIENTRY  glTexParameteriv(GLenum target, GLenum pname, const GLint *params) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texParams(target,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xTexParameteriv(ctx->getES1xContext(), target, pname, params);
  return;
}

GL_API void GL_APIENTRY  glTexParameterx(GLenum target, GLenum pname, GLfixed param) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texParams(target,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xTexParameterx(ctx->getES1xContext(), target, pname, param);
  return;
}

GL_API void GL_APIENTRY  glTexParameterxv(GLenum target, GLenum pname, const GLfixed *params) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texParams(target,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p --->", __func__, ctx);
  es1xTexParameterxv(ctx->getES1xContext(), target, pname, params);
  return;
}

GL_API void GL_APIENTRY  glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset,
                                         GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
  GET_CTX_CM();
  SET_ERROR_IF(!(GLEScmValidate::textureTargetEx(target) &&
                 GLEScmValidate::pixelFrmt(ctx,format)&&
                 GLEScmValidate::pixelType(ctx,type)),GL_INVALID_ENUM);
  SET_ERROR_IF(!GLEScmValidate::pixelOp(format,type),GL_INVALID_OPERATION);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xTexSubImage2D(ctx->getES1xContext(), target, level, xoffset, yoffset,
                    width, height, format, type, pixels);
  return;

}

GL_API void GL_APIENTRY  glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xTranslatef(ctx->getES1xContext(), x, y, z);
  return;
}

GL_API void GL_APIENTRY  glTranslatex(GLfixed x, GLfixed y, GLfixed z) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xTranslatex(ctx->getES1xContext(), x, y, z);
  return;
}


GL_API void GL_APIENTRY  glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::vertexPointerParams(size,stride),GL_INVALID_VALUE);
  SET_ERROR_IF(!GLEScmValidate::vertexPointerType(type),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xVertexPointer(ctx->getES1xContext(), size, type, stride, pointer);
  return;
}

GL_API void GL_APIENTRY  glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  es1xViewport(ctx->getES1xContext(), x, y, width, height);
  return;
}

GL_API void GL_APIENTRY glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image)
{
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::textureTargetLimited(target),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> short-circuited to GLES_V2: \n", __func__, ctx);

  v2_glEGLImageTargetTexture2DOES(target, image);
}

GL_API void GL_APIENTRY glEGLImageTargetRenderbufferStorageOES(GLenum target, GLeglImageOES image)
{
  GET_CTX_CM();
  SET_ERROR_IF(target != GL_RENDERBUFFER_OES,GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> short-circuited to GLES_V2: \n", __func__, ctx);

  v2_glEGLImageTargetRenderbufferStorageOES(target, image);
}

/* GL_OES_blend_subtract*/
GL_API void GL_APIENTRY glBlendEquationOES(GLenum mode) {
  GET_CTX_CM();
  SET_ERROR_IF(!(GLEScmValidate::blendEquationMode(mode)), GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> EXTENSION with GLESv2 equivalent: ", __func__, ctx);
  es1xBlendEquation(ctx->getES1xContext(), mode);
}

/* GL_OES_blend_equation_separate */
GL_API void GL_APIENTRY glBlendEquationSeparateOES (GLenum modeRGB, GLenum modeAlpha) {
  GET_CTX_CM();
  SET_ERROR_IF(!(GLEScmValidate::blendEquationMode(modeRGB) && GLEScmValidate::blendEquationMode(modeAlpha)), GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> EXTENSION with GLESv2 equivalent: ", __func__, ctx);
  es1xBlendEquationSeparate(ctx->getES1xContext(), modeRGB, modeAlpha);
}

/* GL_OES_blend_func_separate */
GL_API void GL_APIENTRY glBlendFuncSeparateOES(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::blendSrc(srcRGB) || !GLEScmValidate::blendDst(dstRGB) ||
               !GLEScmValidate::blendSrc(srcAlpha) || ! GLEScmValidate::blendDst(dstAlpha) ,GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> EXTENSION with GLESv2 equivalent: ", __func__, ctx);
  es1xBlendFuncSeparate(ctx->getES1xContext(), srcRGB, dstRGB, srcAlpha, dstAlpha);
}

/* GL_OES_framebuffer_object */
GL_API GLboolean GL_APIENTRY glIsRenderbufferOES(GLuint renderbuffer) {
  GET_CTX_CM_RET(GL_FALSE);
  RET_AND_SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION,GL_FALSE);
  fprintf(stdout, "v1: %s ctx=%p ---> UNIMPLEMENTED\n", __func__, ctx); return GL_FALSE;
  // if(renderbuffer && ctx->shareGroup().Ptr()){
  //   return ctx->shareGroup()->isObject(RENDERBUFFER,renderbuffer) ? GL_TRUE :GL_FALSE;
  // }
  // return ctx->dispatcher().glIsRenderbufferEXT(renderbuffer);
}

GL_API void GLAPIENTRY glBindRenderbufferOES(GLenum target, GLuint renderbuffer) {
  GET_CTX_CM();
  SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
  SET_ERROR_IF(!GLEScmValidate::renderbufferTarget(target),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> UNIMPLEMENTED\n", __func__, ctx);
  // //if buffer wasn't generated before,generate one
  // if(renderbuffer && ctx->shareGroup().Ptr() && !ctx->shareGroup()->isObject(RENDERBUFFER,renderbuffer)){
  //   ctx->shareGroup()->genName(RENDERBUFFER,renderbuffer);
  //   ctx->shareGroup()->setObjectData(RENDERBUFFER,
  //                                    renderbuffer,
  //                                    ObjectDataPtr(new RenderbufferData()));
  // }

  // int globalBufferName = (renderbuffer != 0) ? ctx->shareGroup()->getGlobalName(RENDERBUFFER,renderbuffer) : 0;
  // ctx->dispatcher().glBindRenderbufferEXT(target,globalBufferName);

  // // update renderbuffer binding state
  // ctx->setRenderbufferBinding(renderbuffer);
}


GL_API void GLAPIENTRY glDeleteRenderbuffersOES(GLsizei n, const GLuint *renderbuffers) {
  GET_CTX_CM();
  SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
  fprintf(stdout, "v1: %s ctx=%p ---> UNIMPLEMENTED\n", __func__, ctx);
  //   for (int i=0;i<n;++i) {
  //     GLuint globalBufferName = ctx->shareGroup()->getGlobalName(RENDERBUFFER,renderbuffers[i]);
  //     ctx->dispatcher().glDeleteRenderbuffersEXT(1,&globalBufferName);
  //   }
}

GL_API void GLAPIENTRY glGenRenderbuffersOES(GLsizei n, GLuint *renderbuffers) {
  GET_CTX_CM();
  SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
  SET_ERROR_IF(n<0,GL_INVALID_VALUE);
  fprintf(stdout, "v1: %s ctx=%p ---> UNIMPLEMENTED\n", __func__, ctx);
  //   if(ctx->shareGroup().Ptr()) {
  //     for(int i=0; i<n ;i++) {
  //       renderbuffers[i] = ctx->shareGroup()->genName(RENDERBUFFER, 0, true);
  //       ctx->shareGroup()->setObjectData(RENDERBUFFER,
  //                                        renderbuffers[i],
  //                                        ObjectDataPtr(new RenderbufferData()));
  //     }
  //   }
}

GL_API void GLAPIENTRY glRenderbufferStorageOES(GLenum target, GLenum internalformat, GLsizei width, GLsizei height){
  GET_CTX_CM();
  SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
  SET_ERROR_IF(!GLEScmValidate::renderbufferTarget(target) || !GLEScmValidate::renderbufferInternalFrmt(ctx,internalformat) ,GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> UNIMPLEMENTED\n", __func__, ctx);
  //   if (internalformat==GL_RGB565_OES) //RGB565 not supported by GL
  //     internalformat = GL_RGB8_OES;

  //   // Get current bounded renderbuffer
  //   // raise INVALID_OPERATIOn if no renderbuffer is bounded
  //   GLuint rb = ctx->getRenderbufferBinding();
  //   SET_ERROR_IF(rb == 0,GL_INVALID_OPERATION);
  //   ObjectDataPtr objData = ctx->shareGroup()->getObjectData(RENDERBUFFER,rb);
  //   RenderbufferData *rbData = (RenderbufferData *)objData.Ptr();
  //   SET_ERROR_IF(!rbData,GL_INVALID_OPERATION);

  //   //
  //   // if the renderbuffer was an eglImage target, detach from
  //   // the eglImage.
  //   //
  //   if (rbData->sourceEGLImage != 0) {
  //     if (rbData->eglImageDetach) {
  //       (*rbData->eglImageDetach)(rbData->sourceEGLImage);
  //     }
  //     rbData->sourceEGLImage = 0;
  //     rbData->eglImageGlobalTexName = 0;
  //   }

  //   ctx->dispatcher().glRenderbufferStorageEXT(target,internalformat,width,height);
}

GL_API void GLAPIENTRY glGetRenderbufferParameterivOES(GLenum target, GLenum pname, GLint* params) {
  GET_CTX_CM();
  SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
  SET_ERROR_IF(!GLEScmValidate::renderbufferTarget(target) || !GLEScmValidate::renderbufferParams(pname) ,GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> UNIMPLEMENTED\n", __func__, ctx);

  //   //
  //   // If this is a renderbuffer which is eglimage's target, we
  //   // should query the underlying eglimage's texture object instead.
  //   //
  //   GLuint rb = ctx->getRenderbufferBinding();
  //   if (rb) {
  //     ObjectDataPtr objData = ctx->shareGroup()->getObjectData(RENDERBUFFER,rb);
  //     RenderbufferData *rbData = (RenderbufferData *)objData.Ptr();
  //     if (rbData && rbData->sourceEGLImage != 0) {
  //       GLenum texPname;
  //       switch(pname) {
  //         case GL_RENDERBUFFER_WIDTH_OES:
  //           texPname = GL_TEXTURE_WIDTH;
  //           break;
  //         case GL_RENDERBUFFER_HEIGHT_OES:
  //           texPname = GL_TEXTURE_HEIGHT;
  //           break;
  //         case GL_RENDERBUFFER_INTERNAL_FORMAT_OES:
  //           texPname = GL_TEXTURE_INTERNAL_FORMAT;
  //           break;
  //         case GL_RENDERBUFFER_RED_SIZE_OES:
  //           texPname = GL_TEXTURE_RED_SIZE;
  //           break;
  //         case GL_RENDERBUFFER_GREEN_SIZE_OES:
  //           texPname = GL_TEXTURE_GREEN_SIZE;
  //           break;
  //         case GL_RENDERBUFFER_BLUE_SIZE_OES:
  //           texPname = GL_TEXTURE_BLUE_SIZE;
  //           break;
  //         case GL_RENDERBUFFER_ALPHA_SIZE_OES:
  //           texPname = GL_TEXTURE_ALPHA_SIZE;
  //           break;
  //         case GL_RENDERBUFFER_DEPTH_SIZE_OES:
  //           texPname = GL_TEXTURE_DEPTH_SIZE;
  //           break;
  //         case GL_RENDERBUFFER_STENCIL_SIZE_OES:
  //         default:
  //           *params = 0; //XXX
  //           return;
  //           break;
  //       }

  //       GLint prevTex;
  //       ctx->dispatcher().glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
  //       ctx->dispatcher().glBindTexture(GL_TEXTURE_2D,
  //                                   rbData->eglImageGlobalTexName);
  //       ctx->dispatcher().glGetTexLevelParameteriv(GL_TEXTURE_2D, 0,
  //                                              texPname,
  //                                              params);
  //       ctx->dispatcher().glBindTexture(GL_TEXTURE_2D, prevTex);
  //       return;
  //     }
  //  }

  //   ctx->dispatcher().glGetRenderbufferParameterivEXT(target,pname,params);
}

GL_API GLboolean GLAPIENTRY glIsFramebufferOES(GLuint framebuffer) {
  GET_CTX_RET(GL_FALSE);
  RET_AND_SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION,GL_FALSE);
  fprintf(stdout, "v1: %s ctx=%p ---> UNIMPLEMENTED\n", __func__, ctx); return GL_FALSE;
  //   if (framebuffer && ctx->shareGroup().Ptr()) {
  //     return ctx->shareGroup()->isObject(FRAMEBUFFER,framebuffer) ? GL_TRUE : GL_FALSE;
  //   }
  //   return ctx->dispatcher().glIsFramebufferEXT(framebuffer);
}

GL_API void GLAPIENTRY glBindFramebufferOES(GLenum target, GLuint framebuffer) {
  GET_CTX_CM();
  SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
  SET_ERROR_IF(!GLEScmValidate::framebufferTarget(target) ,GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> UNIMPLEMENTED\n", __func__, ctx);
  //   if (framebuffer && ctx->shareGroup().Ptr() && !ctx->shareGroup()->isObject(FRAMEBUFFER,framebuffer)) {
  //     ctx->shareGroup()->genName(FRAMEBUFFER,framebuffer);
  //     ctx->shareGroup()->setObjectData(FRAMEBUFFER, framebuffer,
  //                                      ObjectDataPtr(new FramebufferData(framebuffer)));
  //   }
  //   int globalBufferName = (framebuffer!=0) ? ctx->shareGroup()->getGlobalName(FRAMEBUFFER,framebuffer) : 0;
  //   ctx->dispatcher().glBindFramebufferEXT(target,globalBufferName);

  //   // update framebuffer binding state
  //   ctx->setFramebufferBinding(framebuffer);
}

GL_API void GLAPIENTRY glDeleteFramebuffersOES(GLsizei n, const GLuint *framebuffers) {
  GET_CTX_CM();
  SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
  fprintf(stdout, "v1: %s ctx=%p ---> UNIMPLEMENTED\n", __func__, ctx);
  //   for (int i=0;i<n;++i) {
  //     GLuint globalBufferName = ctx->shareGroup()->getGlobalName(FRAMEBUFFER,framebuffers[i]);
  //     ctx->dispatcher().glDeleteFramebuffersEXT(1,&globalBufferName);
  //   }
}

GL_API void GLAPIENTRY glGenFramebuffersOES(GLsizei n, GLuint *framebuffers) {
  GET_CTX_CM();
  SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
  SET_ERROR_IF(n<0,GL_INVALID_VALUE);
  fprintf(stdout, "v1: %s ctx=%p ---> UNIMPLEMENTED\n", __func__, ctx);
  //   if (ctx->shareGroup().Ptr()) {
  //     for (int i=0;i<n;i++) {
  //       framebuffers[i] = ctx->shareGroup()->genName(FRAMEBUFFER, 0, true);
  //       ctx->shareGroup()->setObjectData(FRAMEBUFFER, framebuffers[i],
  //                                        ObjectDataPtr(new FramebufferData(framebuffers[i])));
  //     }
  //   }
}

GL_API GLenum GLAPIENTRY glCheckFramebufferStatusOES(GLenum target) {
  GET_CTX_RET(0);
  RET_AND_SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION,0);
  RET_AND_SET_ERROR_IF(!GLEScmValidate::framebufferTarget(target) ,GL_INVALID_ENUM,0);
  fprintf(stdout, "v1: %s ctx=%p ---> Forwarded directly to libGL\n", __func__, ctx);
  return ctx->dispatcher().glCheckFramebufferStatusEXT(target);
}

GL_API void GLAPIENTRY glFramebufferTexture2DOES(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
  GET_CTX_CM();
  SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
  SET_ERROR_IF(!GLEScmValidate::framebufferTarget(target) || !GLEScmValidate::framebufferAttachment(attachment) ||
               !GLEScmValidate::textureTargetEx(textarget),GL_INVALID_ENUM);
  SET_ERROR_IF(!ctx->shareGroup().Ptr(), GL_INVALID_OPERATION);
  fprintf(stdout, "v1: %s ctx=%p ---> UNIMPLEMENTED\n", __func__, ctx);
  //   GLuint globalTexName = 0;
  //   if(texture) {
  //     if (!ctx->shareGroup()->isObject(TEXTURE,texture)) {
  //       ctx->shareGroup()->genName(TEXTURE,texture);
  //     }
  //     ObjectLocalName texname = TextureLocalName(textarget,texture);
  //     globalTexName = ctx->shareGroup()->getGlobalName(TEXTURE,texname);
  //   }

  //   ctx->dispatcher().glFramebufferTexture2DEXT(target,attachment,textarget,globalTexName,level);

  //   // Update the the current framebuffer object attachment state
  //   GLuint fbName = ctx->getFramebufferBinding();
  //   ObjectDataPtr fbObj = ctx->shareGroup()->getObjectData(FRAMEBUFFER,fbName);
  //   if (fbObj.Ptr() != NULL) {
  //     FramebufferData *fbData = (FramebufferData *)fbObj.Ptr();
  //     fbData->setAttachment(attachment, textarget,
  //                           texture, ObjectDataPtr(NULL));
  //   }
}

GL_API void GLAPIENTRY glFramebufferRenderbufferOES(GLenum target, GLenum attachment,GLenum renderbuffertarget, GLuint renderbuffer) {
  GET_CTX_CM();
  SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
  SET_ERROR_IF(!GLEScmValidate::framebufferTarget(target) ||
               !GLEScmValidate::framebufferAttachment(attachment) ||
               !GLEScmValidate::renderbufferTarget(renderbuffertarget), GL_INVALID_ENUM);

  SET_ERROR_IF(!ctx->shareGroup().Ptr(), GL_INVALID_OPERATION);

  fprintf(stdout, "v1: %s ctx=%p ---> UNIMPLEMENTED\n", __func__, ctx);

  //   GLuint globalBufferName = 0;
  //   ObjectDataPtr obj;

  //   // generate the renderbuffer object if not yet exist
  //   if (renderbuffer) {
  //     if (!ctx->shareGroup()->isObject(RENDERBUFFER,renderbuffer)) {
  //       ctx->shareGroup()->genName(RENDERBUFFER,renderbuffer);
  //       obj = ObjectDataPtr(new RenderbufferData());
  //       ctx->shareGroup()->setObjectData(RENDERBUFFER,
  //                                        renderbuffer,
  //                                        ObjectDataPtr(new RenderbufferData()));
  //     }
  //     else {
  //       obj = ctx->shareGroup()->getObjectData(RENDERBUFFER,renderbuffer);
  //     }
  //     globalBufferName = ctx->shareGroup()->getGlobalName(RENDERBUFFER,renderbuffer);
  //   }

  //   // Update the the current framebuffer object attachment state
  //   GLuint fbName = ctx->getFramebufferBinding();
  //   ObjectDataPtr fbObj = ctx->shareGroup()->getObjectData(FRAMEBUFFER,fbName);
  //   if (fbObj.Ptr() != NULL) {
  //     FramebufferData *fbData = (FramebufferData *)fbObj.Ptr();
  //     fbData->setAttachment(attachment, renderbuffertarget, renderbuffer, obj);
  //   }

  //   if (renderbuffer && obj.Ptr() != NULL) {
  //     RenderbufferData *rbData = (RenderbufferData *)obj.Ptr();
  //     if (rbData->sourceEGLImage != 0) {
  //       //
  //       // This renderbuffer object is an eglImage target
  //       // attach the eglimage's texture instead the renderbuffer.
  //       //
  //       ctx->dispatcher().glFramebufferTexture2DEXT(target,
  //                                               attachment,
  //                                               GL_TEXTURE_2D,
  //                                               rbData->eglImageGlobalTexName,0);
  //       return;
  //     }
  //   }

  //   ctx->dispatcher().glFramebufferRenderbufferEXT(target,attachment,renderbuffertarget,globalBufferName);
}

GL_API void GLAPIENTRY glGetFramebufferAttachmentParameterivOES(GLenum target, GLenum attachment, GLenum pname, GLint *params) {
  GET_CTX_CM();
  SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
  SET_ERROR_IF(!GLEScmValidate::framebufferTarget(target) || !GLEScmValidate::framebufferAttachment(attachment) ||
               !GLEScmValidate::framebufferAttachmentParams(pname), GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> UNIMPLEMENTED\n", __func__, ctx);
  //   //
  //   // Take the attachment attribute from our state - if available
  //   //
  //   GLuint fbName = ctx->getFramebufferBinding();
  //   if (fbName) {
  //     ObjectDataPtr fbObj = ctx->shareGroup()->getObjectData(FRAMEBUFFER,fbName);
  //     if (fbObj.Ptr() != NULL) {
  //       FramebufferData *fbData = (FramebufferData *)fbObj.Ptr();
  //       GLenum target;
  //       GLuint name = fbData->getAttachment(attachment, &target, NULL);
  //       if (pname == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_OES) {
  //         *params = target;
  //         return;
  //       }
  //       else if (pname == GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_OES) {
  //         *params = name;
  //         return;
  //       }
  //     }
  //  }
  //
  //  ctx->dispatcher().glGetFramebufferAttachmentParameterivEXT(target,attachment,pname,params);
}

GL_API void GL_APIENTRY glGenerateMipmapOES(GLenum target) {
  GET_CTX_CM();
  SET_ERROR_IF(!ctx->getCaps()->GL_EXT_FRAMEBUFFER_OBJECT,GL_INVALID_OPERATION);
  SET_ERROR_IF(!GLEScmValidate::textureTargetLimited(target),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> Forwarded directly to libGL\n", __func__, ctx);
  ctx->dispatcher().glGenerateMipmapEXT(target);
}

GL_API void GL_APIENTRY glCurrentPaletteMatrixOES(GLuint index) {
  GET_CTX_CM();
  SET_ERROR_IF(!(ctx->getCaps()->GL_ARB_MATRIX_PALETTE && ctx->getCaps()->GL_ARB_VERTEX_BLEND),GL_INVALID_OPERATION);
  fprintf(stdout, "v1: %s ctx=%p ---> Forwarded directly to libGL\n", __func__, ctx);
  ctx->dispatcher().glCurrentPaletteMatrixARB(index);
}

GL_API void GL_APIENTRY glLoadPaletteFromModelViewMatrixOES() {
  GET_CTX_CM();
  SET_ERROR_IF(!(ctx->getCaps()->GL_ARB_MATRIX_PALETTE && ctx->getCaps()->GL_ARB_VERTEX_BLEND),GL_INVALID_OPERATION);
  fprintf(stdout, "v1: %s ctx=%p ---> Forwarded directly to libGL\n", __func__, ctx);
  GLint matrix[16];
  ctx->dispatcher().glGetIntegerv(GL_MODELVIEW_MATRIX,matrix);
  ctx->dispatcher().glMatrixIndexuivARB(1,(GLuint*)matrix);

}

GL_API void GL_APIENTRY glMatrixIndexPointerOES(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
  GET_CTX_CM();
  SET_ERROR_IF(!(ctx->getCaps()->GL_ARB_MATRIX_PALETTE && ctx->getCaps()->GL_ARB_VERTEX_BLEND),GL_INVALID_OPERATION);
  fprintf(stdout, "v1: %s ctx=%p ---> Forwarded directly to libGL\n", __func__, ctx);
  ctx->dispatcher().glMatrixIndexPointerARB(size,type,stride,pointer);
}

GL_API void GL_APIENTRY glWeightPointerOES(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
  GET_CTX_CM();
  SET_ERROR_IF(!(ctx->getCaps()->GL_ARB_MATRIX_PALETTE && ctx->getCaps()->GL_ARB_VERTEX_BLEND),GL_INVALID_OPERATION);
  fprintf(stdout, "v1: %s ctx=%p ---> Forwarded directly to libGL\n", __func__, ctx);
  ctx->dispatcher().glWeightPointerARB(size,type,stride,pointer);

}

GL_API void GL_APIENTRY glTexGenfOES (GLenum coord, GLenum pname, GLfloat param) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texGen(coord,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> Forwarded directly to libGL\n", __func__, ctx);
  if (coord == GL_TEXTURE_GEN_STR_OES) {
    ctx->dispatcher().glTexGenf(GL_S,pname,param);
    ctx->dispatcher().glTexGenf(GL_T,pname,param);
    ctx->dispatcher().glTexGenf(GL_R,pname,param);
  }
  else
    ctx->dispatcher().glTexGenf(coord,pname,param);
}

GL_API void GL_APIENTRY glTexGenfvOES (GLenum coord, GLenum pname, const GLfloat *params) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texGen(coord,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> Forwarded directly to libGL\n", __func__, ctx);
  if (coord == GL_TEXTURE_GEN_STR_OES) {
    ctx->dispatcher().glTexGenfv(GL_S,pname,params);
    ctx->dispatcher().glTexGenfv(GL_T,pname,params);
    ctx->dispatcher().glTexGenfv(GL_R,pname,params);
  }
  else
    ctx->dispatcher().glTexGenfv(coord,pname,params);
}

GL_API void GL_APIENTRY glTexGeniOES (GLenum coord, GLenum pname, GLint param) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texGen(coord,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> Forwarded directly to libGL\n", __func__, ctx);
  if (coord == GL_TEXTURE_GEN_STR_OES) {
    ctx->dispatcher().glTexGeni(GL_S,pname,param);
    ctx->dispatcher().glTexGeni(GL_T,pname,param);
    ctx->dispatcher().glTexGeni(GL_R,pname,param);
  }
  else
    ctx->dispatcher().glTexGeni(coord,pname,param);
}

GL_API void GL_APIENTRY glTexGenivOES (GLenum coord, GLenum pname, const GLint *params) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texGen(coord,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> Forwarded directly to libGL\n", __func__, ctx);
  if (coord == GL_TEXTURE_GEN_STR_OES) {
    ctx->dispatcher().glTexGeniv(GL_S,pname,params);
    ctx->dispatcher().glTexGeniv(GL_T,pname,params);
    ctx->dispatcher().glTexGeniv(GL_R,pname,params);
  }
  else
    ctx->dispatcher().glTexGeniv(coord,pname,params);
}

GL_API void GL_APIENTRY glTexGenxOES (GLenum coord, GLenum pname, GLfixed param) {
  GET_CTX_CM();
  SET_ERROR_IF(!GLEScmValidate::texGen(coord,pname),GL_INVALID_ENUM);
  fprintf(stdout, "v1: %s ctx=%p ---> Forwarded directly to libGL\n", __func__, ctx);
  if (coord == GL_TEXTURE_GEN_STR_OES) {
    ctx->dispatcher().glTexGenf(GL_S,pname,X2F(param));
    ctx->dispatcher().glTexGenf(GL_T,pname,X2F(param));
    ctx->dispatcher().glTexGenf(GL_R,pname,X2F(param));
  }
  else
    ctx->dispatcher().glTexGenf(coord,pname,X2F(param));
}

GL_API void GL_APIENTRY glTexGenxvOES (GLenum coord, GLenum pname, const GLfixed *params) {
  GLfloat tmpParams[1];
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> Forwarded directly to libGL\n", __func__, ctx);
  SET_ERROR_IF(!GLEScmValidate::texGen(coord,pname),GL_INVALID_ENUM);
  tmpParams[0] = X2F(params[0]);
  if (coord == GL_TEXTURE_GEN_STR_OES) {
    ctx->dispatcher().glTexGenfv(GL_S,pname,tmpParams);
    ctx->dispatcher().glTexGenfv(GL_T,pname,tmpParams);
    ctx->dispatcher().glTexGenfv(GL_R,pname,tmpParams);
  }
  else
    ctx->dispatcher().glTexGenfv(coord,pname,tmpParams);
}

GL_API void GL_APIENTRY glGetTexGenfvOES (GLenum coord, GLenum pname, GLfloat *params) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> Forwarded directly to libGL\n", __func__, ctx);
  if (coord == GL_TEXTURE_GEN_STR_OES)
  {
    GLfloat state_s = GL_FALSE;
    GLfloat state_t = GL_FALSE;
    GLfloat state_r = GL_FALSE;
    ctx->dispatcher().glGetTexGenfv(GL_S,pname,&state_s);
    ctx->dispatcher().glGetTexGenfv(GL_T,pname,&state_t);
    ctx->dispatcher().glGetTexGenfv(GL_R,pname,&state_r);
    *params = state_s && state_t && state_r ? GL_TRUE: GL_FALSE;
  }
  else
    ctx->dispatcher().glGetTexGenfv(coord,pname,params);

}

GL_API void GL_APIENTRY glGetTexGenivOES (GLenum coord, GLenum pname, GLint *params) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> Forwarded directly to libGL\n", __func__, ctx);
  if (coord == GL_TEXTURE_GEN_STR_OES)
  {
    GLint state_s = GL_FALSE;
    GLint state_t = GL_FALSE;
    GLint state_r = GL_FALSE;
    ctx->dispatcher().glGetTexGeniv(GL_S,pname,&state_s);
    ctx->dispatcher().glGetTexGeniv(GL_T,pname,&state_t);
    ctx->dispatcher().glGetTexGeniv(GL_R,pname,&state_r);
    *params = state_s && state_t && state_r ? GL_TRUE: GL_FALSE;
  }
  else
    ctx->dispatcher().glGetTexGeniv(coord,pname,params);
}

GL_API void GL_APIENTRY glGetTexGenxvOES (GLenum coord, GLenum pname, GLfixed *params) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> Forwarded directly to libGL\n", __func__, ctx);
  GLfloat tmpParams[1];
  if (coord == GL_TEXTURE_GEN_STR_OES)
  {
    GLfloat state_s = GL_FALSE;
    GLfloat state_t = GL_FALSE;
    GLfloat state_r = GL_FALSE;
    ctx->dispatcher().glGetTexGenfv(GL_TEXTURE_GEN_S,pname,&state_s);
    ctx->dispatcher().glGetTexGenfv(GL_TEXTURE_GEN_T,pname,&state_t);
    ctx->dispatcher().glGetTexGenfv(GL_TEXTURE_GEN_R,pname,&state_r);
    tmpParams[0] = state_s && state_t && state_r ? GL_TRUE: GL_FALSE;
  }
  else
    ctx->dispatcher().glGetTexGenfv(coord,pname,tmpParams);

  // __kostas__ : is this a bug ? tmpParams has size 1
  //  params[0] = F2X(tmpParams[1]);
  params[0] = F2X(tmpParams[0]);
}

// template <class T, GLenum TypeName>
// void glDrawTexOES (T x, T y, T z, T width, T height) {
//   GET_CTX_CM();

//   SET_ERROR_IF((width<=0 || height<=0),GL_INVALID_VALUE);

//   ctx->drawValidate();

//   int numClipPlanes;

//   GLint viewport[4] = {};
//   z = (z>1 ? 1 : (z<0 ?  0 : z));

//   T vertices[4*3] = {
//     x , y, z,
//     x , static_cast<T>(y+height), z,
//     static_cast<T>(x+width), static_cast<T>(y+height), z,
//     static_cast<T>(x+width), y, z
//   };
//   GLfloat texels[ctx->getMaxTexUnits()][4*2];
//   memset((void*)texels, 0, ctx->getMaxTexUnits()*4*2*sizeof(GLfloat));

//   ctx->dispatcher().glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
//   ctx->dispatcher().glPushAttrib(GL_TRANSFORM_BIT);

//   //setup projection matrix to draw in viewport aligned coordinates
//   ctx->dispatcher().glMatrixMode(GL_PROJECTION);
//   ctx->dispatcher().glPushMatrix();
//   ctx->dispatcher().glLoadIdentity();
//   ctx->dispatcher().glGetIntegerv(GL_VIEWPORT,viewport);
//   ctx->dispatcher().glOrtho(viewport[0],viewport[0] + viewport[2],viewport[1],viewport[1]+viewport[3],0,-1);
//   //setup texture matrix
//   ctx->dispatcher().glMatrixMode(GL_TEXTURE);
//   ctx->dispatcher().glPushMatrix();
//   ctx->dispatcher().glLoadIdentity();
//   //setup modelview matrix
//   ctx->dispatcher().glMatrixMode(GL_MODELVIEW);
//   ctx->dispatcher().glPushMatrix();
//   ctx->dispatcher().glLoadIdentity();
//   //backup vbo's
//   int array_buffer,element_array_buffer;
//   glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&array_buffer);
//   glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&element_array_buffer);
//   ctx->dispatcher().glBindBuffer(GL_ARRAY_BUFFER,0);
//   ctx->dispatcher().glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

//   //disable clip planes
//   ctx->dispatcher().glGetIntegerv(GL_MAX_CLIP_PLANES,&numClipPlanes);
//   for (int i=0;i<numClipPlanes;++i)
//     ctx->dispatcher().glDisable(GL_CLIP_PLANE0+i);

//   int nTexPtrs = 0;
//   for (int i=0;i<ctx->getMaxTexUnits();++i) {
//     if (ctx->isTextureUnitEnabled(GL_TEXTURE0+i)) {
//       TextureData * texData = NULL;
//       unsigned int texname = ctx->getBindedTexture(GL_TEXTURE0+i,GL_TEXTURE_2D);
//       ObjectLocalName tex = TextureLocalName(GL_TEXTURE_2D,texname);
//       ctx->dispatcher().glClientActiveTexture(GL_TEXTURE0+i);
//       ObjectDataPtr objData = ctx->shareGroup()->getObjectData(TEXTURE,tex);
//       if (objData.Ptr()) {
//         texData = (TextureData*)objData.Ptr();
//         //calculate texels
//         texels[i][0] = (float)(texData->crop_rect[0])/(float)(texData->width);
//         texels[i][1] = (float)(texData->crop_rect[1])/(float)(texData->height);

//         texels[i][2] = (float)(texData->crop_rect[0])/(float)(texData->width);
//         texels[i][3] = (float)(texData->crop_rect[3]+texData->crop_rect[1])/(float)(texData->height);

//         texels[i][4] = (float)(texData->crop_rect[2]+texData->crop_rect[0])/(float)(texData->width);
//         texels[i][5] = (float)(texData->crop_rect[3]+texData->crop_rect[1])/(float)(texData->height);

//         texels[i][6] = (float)(texData->crop_rect[2]+texData->crop_rect[0])/(float)(texData->width);
//         texels[i][7] = (float)(texData->crop_rect[1])/(float)(texData->height);

//         ctx->dispatcher().glTexCoordPointer(2,GL_FLOAT,0,texels[i]);
//         nTexPtrs++;
//       }
//     }
//   }

//   if (nTexPtrs>0) {
//     //draw rectangle - only if we have some textures enabled & ready
//     ctx->dispatcher().glEnableClientState(GL_VERTEX_ARRAY);
//     ctx->dispatcher().glVertexPointer(3,TypeName,0,vertices);
//     ctx->dispatcher().glEnableClientState(GL_TEXTURE_COORD_ARRAY);
//     ctx->dispatcher().glDrawArrays(GL_TRIANGLE_FAN,0,4);
//   }

//   //restore vbo's
//   ctx->dispatcher().glBindBuffer(GL_ARRAY_BUFFER,array_buffer);
//   ctx->dispatcher().glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,element_array_buffer);

//   //restore matrix state

//   ctx->dispatcher().glMatrixMode(GL_MODELVIEW);
//   ctx->dispatcher().glPopMatrix();
//   ctx->dispatcher().glMatrixMode(GL_TEXTURE);
//   ctx->dispatcher().glPopMatrix();
//   ctx->dispatcher().glMatrixMode(GL_PROJECTION);
//   ctx->dispatcher().glPopMatrix();

//   ctx->dispatcher().glPopAttrib();
//   ctx->dispatcher().glPopClientAttrib();
// }

template <class T, GLenum TypeName>
void glDrawTexOES (T x, T y, T z, T width, T height) {
  GET_CTX_CM();

  SET_ERROR_IF((width<=0 || height<=0),GL_INVALID_VALUE);

  ctx->drawValidate();

  fprintf(stdout, "v1: %s ctx=%p ___UNIMPLEMENTED___ \n", __func__, ctx);

  return;
}

GL_API void GL_APIENTRY glDrawTexsOES (GLshort x, GLshort y, GLshort z, GLshort width, GLshort height) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> does not have ES1X counterpart) \n", __func__, ctx);
  glDrawTexOES<GLshort,GL_SHORT>(x,y,z,width,height);
  return;
}

GL_API void GL_APIENTRY glDrawTexiOES (GLint x, GLint y, GLint z, GLint width, GLint height) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ) ", __func__, ctx);
  glDrawTexOES<GLint,GL_INT>(x,y,z,width,height);
  return;
}

GL_API void GL_APIENTRY glDrawTexfOES (GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  glDrawTexOES<GLfloat,GL_FLOAT>(x,y,z,width,height);
  return;
}

GL_API void GL_APIENTRY glDrawTexxOES (GLfixed x, GLfixed y, GLfixed z, GLfixed width, GLfixed height) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  glDrawTexOES<GLfloat,GL_FLOAT>(X2F(x),X2F(y),X2F(z),X2F(width),X2F(height));
  return;
}

GL_API void GL_APIENTRY glDrawTexsvOES (const GLshort * coords) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  glDrawTexOES<GLshort,GL_SHORT>(coords[0],coords[1],coords[2],coords[3],coords[4]);
  return;
}

GL_API void GL_APIENTRY glDrawTexivOES (const GLint * coords) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  glDrawTexOES<GLint,GL_INT>(coords[0],coords[1],coords[2],coords[3],coords[4]);
  return;
}

GL_API void GL_APIENTRY glDrawTexfvOES (const GLfloat * coords) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  glDrawTexOES<GLfloat,GL_FLOAT>(coords[0],coords[1],coords[2],coords[3],coords[4]);
  return;
}

GL_API void GL_APIENTRY glDrawTexxvOES (const GLfixed * coords) {
  GET_CTX_CM();
  fprintf(stdout, "v1: %s ctx=%p ---> ", __func__, ctx);
  glDrawTexOES<GLfloat,GL_FLOAT>(X2F(coords[0]),X2F(coords[1]),X2F(coords[2]),X2F(coords[3]),X2F(coords[4]));
  return;
}
