
#include "SyncThread.h"

#include "DispatchTables.h"
#include "FrameBuffer.h"

#include "OpenGLESDispatch/EGLDispatch.h"

#include "android/utils/debug.h"

#define DEBUG 1

#if DEBUG
#define DPRINT(...) do { \
    if (!VERBOSE_CHECK(syncthreads)) VERBOSE_ENABLE(syncthreads); \
    VERBOSE_TID_FUNCTION_DPRINT(syncthreads, __VA_ARGS__); \
} while(0)
#else
#define DPRINT(...)
#endif

SyncThread::SyncThread() {
    DPRINT("ctor");
    FrameBuffer *fb = FrameBuffer::getFB();
    mDisplay = fb->getDisplay();
    mParentContext = EGL_NO_CONTEXT;
    mSyncContext = EGL_NO_CONTEXT;
    this->start();
}

void SyncThread::createSyncContext() {
    DPRINT("enter");
    SyncThreadCmd to_send;
    to_send.opCode = SYNC_THREAD_INIT;
    sendAsync(to_send);
    DPRINT("exit");
}

void SyncThread::triggerWait(void* glsync,
                             uint32_t timeline) {
    DPRINT("glsync=%p timeline=0x%lx",
            glsync, timeline);
    SyncThreadCmd to_send;
    to_send.opCode = SYNC_THREAD_WAIT;
    sendAndWaitForResult(to_send);
    DPRINT("exit");
}

void SyncThread::triggerExit() {
    DPRINT("enter");
    SyncThreadCmd to_send;
    to_send.opCode = SYNC_THREAD_WAIT;
    sendAndWaitForResult(to_send);
    DPRINT("exit");
}

void SyncThread::doSyncContextCreation() {
   EGLint err = EGL_SUCCESS;

   static const EGLint kTrivialAttribList[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLConfig trivial_config;
    EGLint numConfigs;
    EGLBoolean config_ret =
        s_egl.eglChooseConfig(mDisplay, kTrivialAttribList,
                                  &trivial_config, 1, &numConfigs);
    err = s_egl.eglGetError();
    if (config_ret == EGL_FALSE ||
        err != EGL_SUCCESS) {
        DPRINT("error choosing config! egl err=0x%x", err);
    }

    static const EGLint kContextAttribList[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    mSyncContext =
        s_egl.eglCreateContext(mDisplay, trivial_config,
                                   mParentContext, kContextAttribList);

    err = s_egl.eglGetError();
    if (mSyncContext == EGL_NO_CONTEXT ||
        err != EGL_SUCCESS) {
        DPRINT("error creating context! egl err=0x%x", err);
    }

    static const EGLint pbuf_attribs[] = {
        EGL_WIDTH, 1, EGL_HEIGHT, 1,
        EGL_NONE
    };

    EGLSurface surf =
        s_egl.eglCreatePbufferSurface(mDisplay, trivial_config, pbuf_attribs);

    err = s_egl.eglGetError();
    if (err != EGL_SUCCESS) {
        DPRINT("error creating surface! egl err=0x%x", err);
    }

    EGLBoolean makecurrent_ret =
        s_egl.eglMakeCurrent(mDisplay, surf, surf, mSyncContext);

    err = s_egl.eglGetError();
    if (makecurrent_ret == EGL_FALSE ||
        err != EGL_SUCCESS) {
        DPRINT("error setting context! egl err=0x%x", __FUNCTION__, err);
    }
}

GLint SyncThread::doSyncWait(void* glsync, uint32_t timeline_handle) {
    DPRINT("enter");
    GLint err = s_gles2.glGetError();
    DPRINT("exit");
    return err;
}

GLint SyncThread::doSyncThreadCmd(SyncThreadCmd* cmd) {
    DPRINT("enter");
    GLint result = 0;
    switch (cmd->opCode) {
    case SYNC_THREAD_INIT:
        doSyncContextCreation();
        break;
    case SYNC_THREAD_WAIT:
        doSyncWait(cmd->glsync, cmd->timelineHandle);
        break;
    case SYNC_THREAD_EXIT:
        break;
    }
    DPRINT("exit");
    return result;
}

intptr_t SyncThread::main() {
    DPRINT("in sync thread");

    while (true) {
        SyncThreadCmd cmd = {};
        DPRINT("waiting to receive command");
        mInput.receive(&cmd);
        bool need_reply = cmd.needReply;
        GLint result = doSyncThreadCmd(&cmd);
        if (need_reply) {
            DPRINT("sending result");
            mOutput.send(result);
            DPRINT("sent result");
        }
    }

    return 0;
}

GLint SyncThread::sendAndWaitForResult(SyncThreadCmd& cmd) {
    DPRINT("send with opcode=%d", cmd.opCode);
    cmd.needReply = true;
    mInput.send(cmd);
    GLint result = -1;
    mOutput.receive(&result);
    DPRINT("result=%d", result);
    return result;
}

void SyncThread::sendAsync(SyncThreadCmd& cmd) {
    DPRINT("send with opcode=%d", cmd.opCode);
    cmd.needReply = false;
    mInput.send(cmd);
}

SyncThread* createSyncThread() {
    return new SyncThread();
}

