/*
* Copyright (C) 2017 The Android Open Source Project
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

#include "BlitThread.h"

#include "ColorBuffer.h"
#include "DispatchTables.h"
#include "FrameBuffer.h"
#include "RenderThreadInfo.h"
#include "OpenGLESDispatch/EGLDispatch.h"

#define DEBUG 0

#if DEBUG

#define D(fmt,...) do { \
    fprintf(stderr, "BlitThread: " fmt " curr cxt: 0x%llx\n", ##__VA_ARGS__, (unsigned long long)(uintptr_t)mCurrContext); \
} while (0) \

#else
#define D(fmt,...)
#endif

BlitThread::BlitThread(
        FrameBuffer* fb,
        EGLDisplay display,
            EGLContext pbufcontext,
            EGLSurface pbufsurfaceHandle,
            EGLContext subwincontext,
            EGLSurface subwinsurfaceHandle
            ) : 
    mFb(fb), mDisplay(display),
    mPbufContext(pbufcontext),
    mPbufSurf(pbufsurfaceHandle),
    mSubwinContext(subwincontext),
    mSubwinSurf(subwinsurfaceHandle)
{
    this->start();
}

void BlitThread::bindPbufContext() {
    if (mCurrContext == mPbufContext) return;
    FrameBuffer::getFB()->lock();
    EGLint res = s_egl.eglMakeCurrent(mDisplay, mPbufSurf, mPbufSurf, mPbufContext);
    fprintf(stderr, "%s: res =%d\n", __func__, res);
    if (!res) abort();
    mCurrContext = mPbufContext;
    FrameBuffer::getFB()->unlock();
}

void BlitThread::bindSubwinContext() {
    if (mCurrContext == mSubwinContext) return;
    FrameBuffer::getFB()->lock();
    EGLint res = s_egl.eglMakeCurrent(mDisplay, mSubwinSurf, mSubwinSurf, mSubwinContext);
    fprintf(stderr, "%s: res =%d\n", __func__, res);
    if (!res) abort();
    mCurrContext = mSubwinContext;
    FrameBuffer::getFB()->unlock();
}

void BlitThread::unbindContext() {
    FrameBuffer::getFB()->lock();
    EGLint res = s_egl.eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    fprintf(stderr, "%s: res =%d\n", __func__, res);
    if (!res) abort();
    FrameBuffer::getFB()->unlock();
}

bool BlitThread::transformSubWindow(
            int wx, int wy,
            int ww, int wh,
            int fbw, int fbh, float dpr, float zRot) {
    android::base::AutoLock lock(mCmdLock);
    mCmd.opCode = TransformSubwindow;
    mCmd.subwindowTransformCreateInfo.wx = wx;
    mCmd.subwindowTransformCreateInfo.wy = wy;
    mCmd.subwindowTransformCreateInfo.ww = ww;
    mCmd.subwindowTransformCreateInfo.wh = wh;
    mCmd.subwindowTransformCreateInfo.fbw = fbw;
    mCmd.subwindowTransformCreateInfo.fbh = fbh;
    mCmd.subwindowTransformCreateInfo.dpr = dpr;
    mCmd.subwindowTransformCreateInfo.zRot = zRot;
    mInput.send(mCmd);
    mOutput.receive(&mCmdRes);
    return (bool)mCmdRes;
}


void BlitThread::flush(uint32_t windowSurface) {
    android::base::AutoLock lock(mCmdLock);
    mCmd.opCode = Blit;
    mCmd.windowSurface = windowSurface;
    mInput.send(mCmd);
}

void BlitThread::post(uint32_t colorBuffer) {
    android::base::AutoLock lock(mCmdLock);
    mCmd.opCode = Post;
    mCmd.colorBuffer = colorBuffer;
    mInput.send(mCmd);
}

void BlitThread::set(uint32_t windowSurface, uint32_t colorBuffer) {
    android::base::AutoLock lock(mCmdLock);
    mCmd.opCode = Set;
    mCmd.windowSurface = windowSurface;
    mCmd.colorBuffer = colorBuffer;
    mInput.send(mCmd);
}

uint32_t BlitThread::createColorBuffer(uint32_t width, uint32_t height, GLenum internalFormat, FrameworkFormat frameworkFormat) {
    android::base::AutoLock lock(mCmdLock);
    mCmd.opCode = CreateColorBuffer;
    mCmd.colorBufferCreateInfo.width = width;
    mCmd.colorBufferCreateInfo.height = height;
    mCmd.colorBufferCreateInfo.internalFormat = internalFormat;
    mCmd.colorBufferCreateInfo.frameworkFormat = frameworkFormat;
    mInput.send(mCmd);
    mOutput.receive(&mCmdRes);
    return (uint32_t)mCmdRes;
}

int BlitThread::openColorBuffer(uint32_t colorBuffer) {
    android::base::AutoLock lock(mCmdLock);
    mCmd.opCode = OpenColorBuffer;
    mCmd.colorBuffer = colorBuffer;
    mInput.send(mCmd);
    mOutput.receive(&mCmdRes);
    return (int)mCmdRes;
}

void BlitThread::closeColorBuffer(uint32_t colorBuffer) {
    android::base::AutoLock lock(mCmdLock);
    mCmd.opCode = CloseColorBuffer;
    mCmd.colorBuffer = colorBuffer;
    mInput.send(mCmd);
}

void BlitThread::readColorBuffer(
        uint32_t colorBuffer,
        GLint x, GLint y,
        GLint width, GLint height,
        GLenum format, GLenum type, void* pixels) {
    android::base::AutoLock lock(mCmdLock);
    mCmd.opCode = ReadColorBuffer;
    mCmd.colorBuffer = colorBuffer;
    mCmd.colorBufferOp.x = x;
    mCmd.colorBufferOp.y = y;
    mCmd.colorBufferOp.width = width;
    mCmd.colorBufferOp.height = height;
    mCmd.colorBufferOp.format = format;
    mCmd.colorBufferOp.type = type;
    mCmd.colorBufferOp.pixels = pixels;
    mInput.send(mCmd);
    mOutput.receive(&mCmdRes);
}

void BlitThread::updateColorBuffer(uint32_t colorBuffer,
                       GLint x, GLint y,
                       GLint width, GLint height,
                       GLenum format, GLenum type, void* pixels) {
    android::base::AutoLock lock(mCmdLock);
    mCmd.opCode = UpdateColorBuffer;
    mCmd.colorBuffer = colorBuffer;
    mCmd.colorBufferOp.x = x;
    mCmd.colorBufferOp.y = y;
    mCmd.colorBufferOp.width = width;
    mCmd.colorBufferOp.height = height;
    mCmd.colorBufferOp.format = format;
    mCmd.colorBufferOp.type = type;
    mCmd.colorBufferOp.pixels = pixels;
    mInput.send(mCmd);
    mOutput.receive(&mCmdRes);
}

uint32_t BlitThread::createWindowSurface(int config, int width, int height) {
    android::base::AutoLock lock(mCmdLock);
    mCmd.opCode = CreateWindowSurface;
    mCmd.windowSurfaceCreateInfo.config = config;
    mCmd.windowSurfaceCreateInfo.width = width;
    mCmd.windowSurfaceCreateInfo.height = height;
    mInput.send(mCmd);
    mOutput.receive(&mCmdRes);
    return (uint32_t)mCmdRes;
}

void BlitThread::destroyWindowSurface(uint32_t surface) {
    android::base::AutoLock lock(mCmdLock);
    mCmd.opCode = DestroyWindowSurface;
    mCmd.windowSurface = surface;
    mInput.send(mCmd);
}

void BlitThread::cleanupProcGLObjects(uint64_t puid) {
    android::base::AutoLock lock(mCmdLock);
    mCmd.opCode = CleanupProcGLObjects;
    mCmd.cleanupOp.puid = puid;
    mInput.send(mCmd);
    mOutput.receive(&mCmdRes);
}

uint64_t BlitThread::createSync(bool hasNativeFence, bool destroyWhenSignaled) {
    android::base::AutoLock lock(mCmdLock);
    mCmd.opCode = CreateSync;
    mCmd.fenceSyncCreateInfo.hasNativeFence = hasNativeFence;
    mCmd.fenceSyncCreateInfo.destroyWhenSignaled = destroyWhenSignaled;
    mInput.send(mCmd);
    mOutput.receive(&mCmdRes);
    return mCmdRes;
}

uint64_t BlitThread::getSyncThread() {
    if (!mSyncThread)
        mSyncThread = SyncThread::getSyncThread();
    return (uint64_t)(uintptr_t)mSyncThread;
}

void BlitThread::exit() {
    android::base::AutoLock lock(mCmdLock);
    mCmd.opCode = Exit;
    mInput.send(mCmd);
    mOutput.receive(&mCmdRes);
}

intptr_t BlitThread::main() {
    mTLS = new RenderThreadInfo();

    bool exiting = false;

    BlitThreadCmd cmd = {};
    FenceSync* fence = nullptr;

    bindPbufContext();
    while (!exiting) {
        mInput.receive(&cmd);
        switch (cmd.opCode) {
        case TransformSubwindow:
            D("cmd: TransformSubwindow");
            bindSubwinContext();
            mFb->transformSubWindow(
                    cmd.subwindowTransformCreateInfo.wx,
                    cmd.subwindowTransformCreateInfo.wy,
                    cmd.subwindowTransformCreateInfo.ww,
                    cmd.subwindowTransformCreateInfo.wh,
                    cmd.subwindowTransformCreateInfo.fbw,
                    cmd.subwindowTransformCreateInfo.fbh,
                    cmd.subwindowTransformCreateInfo.dpr,
                    cmd.subwindowTransformCreateInfo.zRot);
            mOutput.send(1);
            break;
        case Blit:
            D("cmd: Blit");
            bindSubwinContext();
            mFb->flushWindowSurfaceColorBuffer2(cmd.windowSurface);
            break;
        case Post:
            D("cmd: Post");
            mFb->scaleColorBufferToSubWindow(cmd.colorBuffer);
            bindSubwinContext();
            mFb->post(cmd.colorBuffer, false);
            break;
        case Set:
            D("cmd: Set");
            mFb->setWindowSurfaceColorBuffer(cmd.windowSurface, cmd.colorBuffer);
            break;
        case CreateColorBuffer:
            D("cmd: CreateColorBuffer");
            mOutput.send(
                    mFb->createColorBuffer(
                        cmd.colorBufferCreateInfo.width,
                        cmd.colorBufferCreateInfo.height,
                        cmd.colorBufferCreateInfo.internalFormat,
                        cmd.colorBufferCreateInfo.frameworkFormat));
            D("cmd: CreateColorBuffer done");
            break;
        case OpenColorBuffer:
            D("cmd: OpenColorBuffer");
            mOutput.send(mFb->openColorBuffer(cmd.colorBuffer));
            break;
        case CloseColorBuffer:
            D("cmd: CloseColorBuffer");
            mFb->closeColorBuffer(cmd.colorBuffer);
            break;
        case ReadColorBuffer:
            D("cmd: ReadColorBuffer");
            mFb->readColorBuffer(
                    cmd.colorBuffer,
                    cmd.colorBufferOp.x,
                    cmd.colorBufferOp.y,
                    cmd.colorBufferOp.width,
                    cmd.colorBufferOp.height,
                    cmd.colorBufferOp.format,
                    cmd.colorBufferOp.type,
                    cmd.colorBufferOp.pixels);
            mOutput.send(0);
            break;
        case UpdateColorBuffer:
            D("cmd: UpdateColorBuffer");
            mFb->updateColorBuffer(
                    cmd.colorBuffer,
                    cmd.colorBufferOp.x,
                    cmd.colorBufferOp.y,
                    cmd.colorBufferOp.width,
                    cmd.colorBufferOp.height,
                    cmd.colorBufferOp.format,
                    cmd.colorBufferOp.type,
                    cmd.colorBufferOp.pixels);
            mOutput.send(0);
            break;
        case CreateWindowSurface:
            D("cmd: CreateWindowSurface");
            mOutput.send(
                mFb->createWindowSurface(
                    cmd.windowSurfaceCreateInfo.config,
                    cmd.windowSurfaceCreateInfo.width,
                    cmd.windowSurfaceCreateInfo.height));
            D("cmd: CreateWindowSurface done");
            break;
        case DestroyWindowSurface:
            D("cmd: DestroyWindowSurface");
            mFb->DestroyWindowSurface(cmd.windowSurface);
            break;
        case CreateSync:
            D("cmd: CreateSync");
            fence = new FenceSync(
                        cmd.fenceSyncCreateInfo.hasNativeFence,
                        cmd.fenceSyncCreateInfo.destroyWhenSignaled);
            s_gles2.glFlush();
            mOutput.send((uint64_t)(uintptr_t)fence);
            break;
        case CleanupProcGLObjects:
            D("cmd: CleanupProcGLObjects");
            mFb->cleanupProcGLObjectsSelf(cmd.cleanupOp.puid);
            mOutput.send(0);
            break;
        case Exit:
            exiting = true;
            mOutput.send(0);
            break;
        }
    }


    return 0;
}
