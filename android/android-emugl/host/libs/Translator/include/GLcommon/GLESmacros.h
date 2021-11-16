#ifndef GLES_MACROS_H
#define GLES_MACROS_H

#define CHECK_ALL_GL_CALLS 0

#if CHECK_ALL_GL_CALLS
#define CONTEXT_CHECK ScopedGLContextChecker ctx_checker (ctx, __FUNCTION__);
#else
#define CONTEXT_CHECK
#endif

#if defined(__linux__) || defined(__APPLE__)

#include <mutex>
#include "android/base/files/PathUtils.h"
#include "android/base/memory/MemoryTracker.h"
#include "android/utils/debug.h"
#include "emugl/common/misc.h"

#define MEM_TRACE_IF(condition, group)                                  \
    if ((condition)) {                                                  \
        static std::once_flag once_flag;                                \
        const std::string func(__FUNCTION__);                           \
        std::call_once(once_flag, [&func]() {                           \
            if (emugl::getMemoryTracker()) {                            \
                android::base::StringView file(__FILE__);               \
                android::base::StringView baseName;                     \
                android::base::PathUtils::split(file, NULL, &baseName); \
                emugl::getMemoryTracker()->addToGroup(                  \
                        group, baseName.str() + ":" + func);            \
            }                                                           \
        });                                                             \
    }
#else
// windows
#define MEM_TRACE_IF(condition, group)
#endif

#define MEM_TRACE(group) MEM_TRACE_IF(true, group)

#define FAIL_IF(condition, description)                                    \
    if ((condition)) {                                                     \
        VERBOSE_PRINT(gles, "%s:%s:%d error %s\n", __FILE__, __FUNCTION__, \
                      __LINE__, description);                              \
        return;                                                            \
    }

#define RET_AND_FAIL_IF(condition, description, ret)                       \
    if ((condition)) {                                                     \
        VERBOSE_PRINT(gles, "%s:%s:%d error %s\n", __FILE__, __FUNCTION__, \
                      __LINE__, description);                              \
        return ret;                                                        \
    }

#define GET_CTX()                                                             \
    MEM_TRACE_IF(strncmp(__FUNCTION__, "gl", 2) == 0, "EMUGL")                \
    FAIL_IF(!s_eglIface, "null s_eglIface")                                   \
    GLEScontext* ctx = s_eglIface->getGLESContext();                          \
    FAIL_IF(!ctx, "null ctx")                                                 \
    CONTEXT_CHECK

#define GET_CTX_CM()                                                          \
    MEM_TRACE_IF(strncmp(__FUNCTION__, "gl", 2) == 0, "EMUGL")                \
    FAIL_IF(!s_eglIface, "null s_eglIface")                                   \
    GLEScmContext* ctx =                                                      \
            static_cast<GLEScmContext*>(s_eglIface->getGLESContext());        \
    FAIL_IF(!ctx, "null ctx")                                                 \
    CONTEXT_CHECK

#define GET_CTX_V2()                                                          \
    MEM_TRACE_IF(strncmp(__FUNCTION__, "gl", 2) == 0, "EMUGL")                \
    FAIL_IF(!s_eglIface, "null s_eglIface")                                   \
    GLESv2Context* ctx =                                                      \
            static_cast<GLESv2Context*>(s_eglIface->getGLESContext());        \
    FAIL_IF(!ctx, "null ctx")                                                 \
    CONTEXT_CHECK

#define GET_CTX_RET(failure_ret)                                              \
    MEM_TRACE_IF(strncmp(__FUNCTION__, "gl", 2) == 0, "EMUGL")                \
    RET_AND_FAIL_IF(!s_eglIface, "null s_eglIface", failure_ret)              \
    GLEScontext* ctx = s_eglIface->getGLESContext();                          \
    RET_AND_FAIL_IF(!ctx, "null ctx", failure_ret)                            \
    CONTEXT_CHECK

#define GET_CTX_CM_RET(failure_ret)                                           \
    MEM_TRACE_IF(strncmp(__FUNCTION__, "gl", 2) == 0, "EMUGL")                \
    RET_AND_FAIL_IF(!s_eglIface, "null s_eglIface", failure_ret)              \
    GLEScmContext* ctx =                                                      \
            static_cast<GLEScmContext*>(s_eglIface->getGLESContext());        \
    RET_AND_FAIL_IF(!ctx, "null ctx", failure_ret)                            \
    CONTEXT_CHECK

#define GET_CTX_V2_RET(failure_ret)                                           \
    MEM_TRACE_IF(strncmp(__FUNCTION__, "gl", 2) == 0, "EMUGL")                \
    RET_AND_FAIL_IF(!s_eglIface, "null s_eglIface", failure_ret)              \
    GLESv2Context* ctx =                                                      \
            static_cast<GLESv2Context*>(s_eglIface->getGLESContext());        \
    RET_AND_FAIL_IF(!ctx, "null ctx", failure_ret)                            \
    CONTEXT_CHECK

#define SET_ERROR_IF(condition, err)                                         \
    if ((condition)) {                                                       \
        VERBOSE_PRINT(gles, "%s:%s:%d error 0x%x\n", __FILE__, __FUNCTION__, \
                      __LINE__, err);                                        \
        ctx->setGLerror(err);                                                \
        return;                                                              \
    }

#define RET_AND_SET_ERROR_IF(condition, err, ret)                            \
    if ((condition)) {                                                       \
        VERBOSE_PRINT(gles, "%s:%s:%d error 0x%x\n", __FILE__, __FUNCTION__, \
                      __LINE__, err);                                        \
        ctx->setGLerror(err);                                                \
        return ret;                                                          \
    }

#define SET_ERROR_IF_DISPATCHER_NOT_SUPPORT(func) \
            SET_ERROR_IF(!ctx->dispatcher().func, GL_INVALID_OPERATION)

#define RET_AND_SET_ERROR_IF_DISPATCHER_NOT_SUPPORT(func, ret) \
            RET_AND_SET_ERROR_IF(!ctx->dispatcher().func, GL_INVALID_OPERATION, ret)

// Define the following flag to work around cocos2d rendering bug
// BUG: 119568237
#define TOLERATE_PROGRAM_LINK_ERROR 1

#endif
