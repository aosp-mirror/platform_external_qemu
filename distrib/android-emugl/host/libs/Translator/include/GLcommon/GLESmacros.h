#ifndef GLES_MACROS_H
#define GLES_MACROS_H

#include <stdio.h>

#define GET_CTX()                                       \
  if(!s_eglIface)                                       \
    return;                                             \
  GLEScontext *ctx = s_eglIface->getGLESContext();      \
  while(0)

#define GET_CTX_CM()                                                    \
  if(!s_eglIface)                                                       \
    return;                                                             \
  GLEScmContext *ctx = static_cast<GLEScmContext *>(s_eglIface->getGLESContext()); \
  if(!ctx) {                                                            \
    /* fprintf(stdout, "GET_CTX_CM: ctx == NULL @ %s\n", __func__); */  \
    return;                                                             \
  }                                                                     \
  while(0)

#define GET_CTX_V1()                                                    \
  if(!s_eglIface)                                                       \
    return;                                                             \
  GLESv1Context *ctx = static_cast<GLESv1Context *>(s_eglIface->getGLESContext()); \
  if(!ctx) {                                                            \
    /* fprintf(stdout, "GET_CTX_V1: ctx == NULL @ %s\n", __func__); */  \
    return;                                                             \
  }                                                                     \
  while(0)

#define GET_CTX_V2()                                                    \
  if(!s_eglIface)                                                       \
    return;                                                             \
  GLESv2Context *ctx = static_cast<GLESv2Context *>(s_eglIface->getGLESContext()); \
  if(!ctx) {                                                            \
    /* fprintf(stdout, "GET_CTX_V2: ctx == NULL @ %s\n", __func__); */  \
    return;                                                             \
  }                                                                     \
  while(0)

#define GET_CTX_RET(failure_ret)                                        \
  if(!s_eglIface)                                                       \
    return failure_ret;                                                 \
  GLEScontext *ctx = s_eglIface->getGLESContext();                      \
  if(!ctx) {                                                            \
    /* fprintf(stdout, "GET_CTX_RET: ctx == NULL @ %s\n", __func__); */ \
    return failure_ret;                                                 \
  }                                                                     \
  while(0)

#define GET_CTX_CM_RET(failure_ret)                                     \
    if(!s_eglIface)                                                     \
      return failure_ret;                                               \
    GLEScmContext *ctx = static_cast<GLEScmContext *>(s_eglIface->getGLESContext()); \
    if(!ctx) {                                                          \
      /* fprintf(stdout, "GET_CTX_CM_RET: ctx == NULL @ %s\n", __func__); */ \
      return failure_ret;                                               \
    }                                                                   \
    while(0)

#define GET_CTX_V1_RET(failure_ret)                                     \
    if(!s_eglIface)                                                     \
      return failure_ret;                                               \
    GLESv1Context *ctx = static_cast<GLESv1Context *>(s_eglIface->getGLESContext()); \
    if(!ctx) {                                                          \
      /* fprintf(stdout, "GET_CTX_V1_RET: ctx == NULL @ %s\n", __func__); */ \
      return failure_ret;                                               \
    }                                                                   \
    while(0)

#define GET_CTX_V2_RET(failure_ret)                                     \
    if(!s_eglIface)                                                     \
      return failure_ret;                                               \
    GLESv2Context *ctx = static_cast<GLESv2Context *>(s_eglIface->getGLESContext()); \
    if(!ctx) {                                                          \
      /* fprintf(stdout, "GET_CTX_V2_RET: ctx == NULL @ %s\n", __func__); */ \
      return failure_ret;                                               \
    }                                                                   \
    while(0)

#define SET_ERROR_IF(condition,err)                                     \
    if((condition)) {                                                   \
      /* fprintf(stdout, "%s:%s:%d error 0x%x\n", __FILE__, __FUNCTION__, __LINE__, err); */ \
      ctx->setGLerror(err);                                             \
      return;                                                           \
    }                                                                   \
    while(0)


#define RET_AND_SET_ERROR_IF(condition,err,ret)                         \
      if((condition)) {                                                 \
        /* fprintf(stdout, "%s:%s:%d error 0x%x\n", __FILE__, __FUNCTION__, __LINE__, err); */ \
        ctx->setGLerror(err);                                           \
        return ret;                                                     \
      }                                                                 \
      while(0)

#endif
