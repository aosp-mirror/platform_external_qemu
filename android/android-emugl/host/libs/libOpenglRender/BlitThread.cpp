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

void BlitThread::blit(const WindowSurface::FlushColorBufferCmd& flushCmd) {
    BlitThreadCmd blitCmd;
    blitCmd.opCode = BLIT_THREAD_BLIT;
    blitCmd.flushCmd = flushCmd;
    mInput.send(blitCmd);
}

intptr_t BlitThread::main() {
    fprintf(stderr, "%s: start blit thread\n", __func__);
    mTLS = new RenderThreadInfo();
    FrameBuffer::getFB()->createTrivialContext(0, &this->mContext, &this->mSurface);
    FrameBuffer::getFB()->bindContext(mContext, mSurface, mSurface);
    ColorBuffer::bindHelperContext();

    bool exiting = false;

    BlitThreadCmd cmd = {};
    while (!exiting) {
        mInput.receive(&cmd);
        switch (cmd.opCode) {
        case BLIT_THREAD_BLIT:
            cmd.flushCmd.toFlush->blitFromCurrentReadBuffer2();
            break;
        case BLIT_THREAD_EXIT:
            exiting = true;
            break;
        default:
            break;
        }
    }

    ColorBuffer::unbindHelperContext();

    return 0;
}


    
