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

#pragma once

#include "android/base/synchronization/Lock.h"
#include "android/base/synchronization/MessageChannel.h"

#include "emugl/common/thread.h"

#include "FenceSync.h"
#include "SyncThread.h"
#include "WindowSurface.h"

class FrameBuffer;

struct RenderThreadInfo;
class BlitThread : public emugl::Thread {
public:
    BlitThread(
            FrameBuffer* fb,
            EGLDisplay display,
            EGLContext pbufcontext,
            EGLSurface pbufsurfaceHandle,
            EGLContext subwincontext,
            EGLSurface subwinsurfaceHandle
            );

    void bindPbufContext();
    void bindSubwinContext();
    void unbindContext();

    bool transformSubWindow(
            int wx, int wy,
            int ww, int wh,
            int fbw, int fbh, float dpr, float zRot);
    void flush(uint32_t windowSurface);
    void post(uint32_t colorBuffer);
    void set(uint32_t windowSurface, uint32_t colorBuffer);

    uint32_t createColorBuffer(uint32_t width, uint32_t height, GLenum internalFormat, FrameworkFormat frameworkFormat);
    int openColorBuffer(uint32_t colorBuffer);
    void closeColorBuffer(uint32_t colorBuffer);
    void readColorBuffer(uint32_t colorBuffer,
                         GLint x, GLint y,
                         GLint width, GLint height,
                         GLenum format, GLenum type, void* pixels);
    void updateColorBuffer(uint32_t colorBuffer,
                           GLint x, GLint y,
                           GLint width, GLint height,
                           GLenum format, GLenum type, void* pixels);

    uint32_t createWindowSurface(int config, int width, int height);
    void destroyWindowSurface(uint32_t surface);

    void cleanupProcGLObjects(uint64_t puid);

    uint64_t createSync(bool hasNativeFence, bool destroyWhenSignaled);
    uint64_t getSyncThread();
    void exit();

private:
    enum BlitThreadOpCode {
        Blit = 0,
        TransformSubwindow = 1,
        Post = 2,
        Set = 3,
        CreateColorBuffer = 4,
        OpenColorBuffer = 5,
        CloseColorBuffer = 6,
        ReadColorBuffer = 7,
        UpdateColorBuffer = 8,
        CreateWindowSurface = 9,
        DestroyWindowSurface = 10,
        CreateSync = 11,
        CleanupProcGLObjects = 12,
        Exit = 13,
    };
   
    struct ColorBufferCreateInfo {
        GLint width;
        GLint height;
        GLenum internalFormat;
        FrameworkFormat frameworkFormat;
    };

    struct WindowSurfaceCreateInfo {
        int config;
        int width;
        int height;
    };

    struct FenceSyncCreateInfo {
        bool hasNativeFence;
        bool destroyWhenSignaled;
    };

    struct SubwindowTransformCreateInfo {
        int wx; int wy;
        int ww; int wh;
        int fbw; int fbh;
        int dpr;
        int zRot;
    };

    struct ColorBufferOp {
        GLint x; GLint y;
        GLint width; GLint height;
        GLenum format; GLenum type;
        void* pixels;
    };

    struct CleanupOp {
        uint64_t puid;
    };

    struct BlitThreadCmd {
        BlitThreadOpCode opCode = Exit;
        HandleType windowSurface = 0;
        HandleType colorBuffer = 0;
        union {
            ColorBufferCreateInfo colorBufferCreateInfo;
            WindowSurfaceCreateInfo windowSurfaceCreateInfo;
            FenceSyncCreateInfo fenceSyncCreateInfo;
            SubwindowTransformCreateInfo subwindowTransformCreateInfo;
            ColorBufferOp colorBufferOp;
            CleanupOp cleanupOp;
        };
    };

    virtual intptr_t main() override final;
    android::base::MessageChannel<BlitThreadCmd, 16> mInput;
    android::base::MessageChannel<uint64_t, 16> mOutput;

    // internal context members
    RenderThreadInfo* mTLS = nullptr;

    // Sync thread just for this blit thread
    SyncThread* mSyncThread = nullptr;

    // current command
    BlitThreadCmd mCmd = {};
    // current command return code
    uint64_t mCmdRes = 0;
    // lock to set current command, we expect
    // 2 threads at least to be sending
    android::base::Lock mCmdLock;

    FrameBuffer* mFb = nullptr;
    EGLDisplay mDisplay = EGL_NO_DISPLAY;
    EGLContext mPbufContext = EGL_NO_CONTEXT;
    EGLSurface mPbufSurf = 0;
    EGLContext mSubwinContext = EGL_NO_CONTEXT;
    EGLSurface mSubwinSurf = 0;

    // To avoid extra makeCurrent calls
    EGLContext mCurrContext = EGL_NO_CONTEXT;
};
