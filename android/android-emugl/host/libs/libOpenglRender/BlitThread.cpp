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
#include "FrameBuffer.h"
#include "RenderThreadInfo.h"

BlitThread::BlitThread() {
    mDisplay = FrameBuffer::getFB()->getDisplay();
    this->start();
}

void BlitThread::blit2(FenceSync* sync, uint32_t windowSurface) {
    BlitThreadCmd blitCmd;
    blitCmd.opCode = BLIT_THREAD_BLIT;
    blitCmd.windowSurface = windowSurface;
    blitCmd.sync = sync;
    mInput.send(blitCmd);
}

void BlitThread::post(uint32_t colorBuffer) {
    BlitThreadCmd blitCmd;
    blitCmd.opCode = BLIT_THREAD_POST;
    blitCmd.colorBuffer = colorBuffer;
    mInput.send(blitCmd);
}

void BlitThread::setWsCb(uint32_t ws, uint32_t cb) {
    BlitThreadCmd blitCmd;
    blitCmd.opCode = BLIT_THREAD_SETWSCB;
    blitCmd.windowSurface = ws;
    blitCmd.colorBuffer = cb;
    mInput.send(blitCmd);
    mOutput.receive(&blitCmd);
}

intptr_t BlitThread::main() {
    fprintf(stderr, "%s: start blit thread\n", __func__);
    mTLS = new RenderThreadInfo();
    FrameBuffer::getFB()->createTrivialContext(0, &this->mContext, &this->mSurface);
    FrameBuffer::getFB()->bindContext(mContext, mSurface, mSurface);

    bool exiting = false;

    BlitThreadCmd cmd = {};
    while (!exiting) {
        mInput.receive(&cmd);
        switch (cmd.opCode) {
        case BLIT_THREAD_BLIT:
            // cmd.sync->wait(1000000000);
            // cmd.sync->decRef();
            FrameBuffer::getFB()->flushWindowSurfaceColorBuffer2(cmd.windowSurface);
            break;
        case BLIT_THREAD_POST:
            FrameBuffer::getFB()->post(cmd.colorBuffer);
            break;
        case BLIT_THREAD_SETWSCB:
            FrameBuffer::getFB()->setWindowSurfaceColorBuffer(cmd.windowSurface, cmd.colorBuffer);
            mOutput.send(cmd);
            break;
        case BLIT_THREAD_EXIT:
            exiting = true;
            break;
        default:
            break;
        }
    }

    FrameBuffer::getFB()->unbindHelperContext();

    return 0;
}


    
