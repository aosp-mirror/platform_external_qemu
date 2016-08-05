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

#define CHECK_PROGRAM_NAME_ERROR(globalProgramName, program) \
            SET_ERROR_IF(globalProgramName==0                                    \
                    && ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER,\
                        program) != 0, GL_INVALID_OPERATION);                    \
            SET_ERROR_IF(globalProgramName==0, GL_INVALID_VALUE);

#define CHECK_PROGRAM_NAME_ERROR_AND_RETURN(globalProgramName, program, ret) \
            RET_AND_SET_ERROR_IF(globalProgramName==0                            \
                    && ctx->shareGroup()->getGlobalName(NamedObjectType::SHADER,\
                        program) != 0, GL_INVALID_OPERATION, ret);               \
            RET_AND_SET_ERROR_IF(globalProgramName==0, GL_INVALID_VALUE, ret);

#define CHECK_SHADER_NAME_ERROR(globalShaderName, shader) \
            SET_ERROR_IF(globalShaderName==0                                     \
                    && ctx->shareGroup()->getGlobalName(NamedObjectType::PROGRAM,\
                        shader) != 0, GL_INVALID_OPERATION);                    \
            SET_ERROR_IF(globalShaderName==0, GL_INVALID_VALUE);

#endif
