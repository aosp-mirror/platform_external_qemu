#ifndef GLES_MACROS_H
#define GLES_MACROS_H

#define GET_CTX() \
            if(!s_eglIface) return; \
            GLEScontext *ctx = s_eglIface->getGLESContext(); \
            if(!ctx) return;

#define GET_CTX_CM() \
            if(!s_eglIface) return; \
            GLEScmContext *ctx = static_cast<GLEScmContext *>(s_eglIface->getGLESContext()); \
            if(!ctx) return;

#define GET_CTX_V2() \
            if(!s_eglIface) return; \
            GLESv2Context *ctx = static_cast<GLESv2Context *>(s_eglIface->getGLESContext()); \
            if(!ctx) return;

#define GET_CTX_RET(failure_ret) \
            if(!s_eglIface) return failure_ret; \
            GLEScontext *ctx = s_eglIface->getGLESContext(); \
            if(!ctx) return failure_ret;

#define GET_CTX_CM_RET(failure_ret) \
            if(!s_eglIface) return failure_ret; \
            GLEScmContext *ctx = static_cast<GLEScmContext *>(s_eglIface->getGLESContext()); \
            if(!ctx) return failure_ret;

#define GET_CTX_V2_RET(failure_ret) \
            if(!s_eglIface) return failure_ret; \
            GLESv2Context *ctx = static_cast<GLESv2Context *>(s_eglIface->getGLESContext()); \
            if(!ctx) return failure_ret;


#define SET_ERROR_IF(condition,err) if((condition)) {                            \
                        fprintf(stderr, "%s:%s:%d error 0x%x\n", __FILE__, __FUNCTION__, __LINE__, err); \
                        ctx->setGLerror(err);                                    \
                        return;                                                  \
                    }


#define RET_AND_SET_ERROR_IF(condition,err,ret) if((condition)) {                \
                        fprintf(stderr, "%s:%s:%d error 0x%x\n", __FILE__, __FUNCTION__, __LINE__, err); \
                        ctx->setGLerror(err);                                    \
                        return ret;                                              \
                    }

#define SET_ERROR_IF_DISPATCHER_NOT_SUPPORT(func) \
            SET_ERROR_IF(!ctx->dispatcher().func, GL_INVALID_OPERATION)

#define RET_AND_SET_ERROR_IF_DISPATCHER_NOT_SUPPORT(func, ret) \
            RET_AND_SET_ERROR_IF(!ctx->dispatcher().func, GL_INVALID_OPERATION, ret)

// Define the following flag to work around cocos2d rendering bug
// BUG: 119568237
#define TOLERATE_PROGRAM_LINK_ERROR 1

#endif
