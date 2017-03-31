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

BlitThread::BlitThread() {
    mDisplay = FrameBuffer::getFB()->getDisplay();
    this->start();
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

uint64_t BlitThread::createSync(bool hasNativeFence, bool destroyWhenSignaled) {
    android::base::AutoLock lock(mCmdLock);
    mCmd.opCode = CreateSync;
    mCmd.fenceSyncCreateInfo.hasNativeFence = hasNativeFence;
    mCmd.fenceSyncCreateInfo.destroyWhenSignaled = destroyWhenSignaled;
    mInput.send(mCmd);
    mOutput.receive(&mCmdRes);
    return mCmdRes;
}

void BlitThread::exit() {
    android::base::AutoLock lock(mCmdLock);
    mCmd.opCode = Exit;
    mInput.send(mCmd);
    mOutput.receive(&mCmdRes);
}

intptr_t BlitThread::main() {
    mTLS = new RenderThreadInfo();
    FrameBuffer* fb = FrameBuffer::getFB();
    fb->createTrivialContext(0, &this->mContext, &this->mSurface);
    fb->bindContext(mContext, mSurface, mSurface);

    bool exiting = false;

    BlitThreadCmd cmd = {};
    FenceSync* fence = nullptr;
    while (!exiting) {
        mInput.receive(&cmd);
        switch (cmd.opCode) {
        case Blit:
            fb->flushWindowSurfaceColorBuffer2(cmd.windowSurface);
            break;
        case Post:
            fb->post(cmd.colorBuffer);
            break;
        case Set:
            fb->setWindowSurfaceColorBuffer(cmd.windowSurface, cmd.colorBuffer);
            break;
        case OpenColorBuffer:
            mOutput.send(fb->openColorBuffer(cmd.colorBuffer));
            break;
        case CloseColorBuffer:
            fb->closeColorBuffer(cmd.colorBuffer);
            break;
        case CreateSync:
            fence = new FenceSync(
                        cmd.fenceSyncCreateInfo.hasNativeFence,
                        cmd.fenceSyncCreateInfo.destroyWhenSignaled);
            s_gles2.glFlush();
            mOutput.send((uint64_t)(uintptr_t)fence);
            break;
        case Exit:
            exiting = true;
            break;
        }
    }

    fb->bindContext(0, 0, 0);
    fb->drainWindowSurface();
    fb->drainRenderContext();
    mOutput.send(0);

    return 0;
}


    
