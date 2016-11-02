/*
* Copyright (C) 2011 The Android Open Source Project
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
#include "RenderThread.h"

#include "ChannelStream.h"
#include "ErrorLog.h"
#include "FrameBuffer.h"
#include "ReadBuffer.h"
#include "RenderControl.h"
#include "RendererImpl.h"
#include "RenderChannelImpl.h"
#include "RenderThreadInfo.h"

#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "OpenGLESDispatch/GLESv1Dispatch.h"
#include "../../../shared/OpenglCodecCommon/ChecksumCalculatorThreadInfo.h"

#include "android/base/system/System.h"

#include <memory>
#include <string.h>

#define STREAM_BUFFER_SIZE 4 * 1024 * 1024

RenderThread::RenderThread(std::weak_ptr<emugl::RendererImpl> renderer,
                           std::shared_ptr<emugl::RenderChannelImpl> channel)
    : mChannel(channel), mRenderer(renderer) {}

RenderThread::~RenderThread() = default;

// static
std::unique_ptr<RenderThread> RenderThread::create(
        std::weak_ptr<emugl::RendererImpl> renderer,
        std::shared_ptr<emugl::RenderChannelImpl> channel) {
    return std::unique_ptr<RenderThread>(
            new RenderThread(renderer, channel));
}

intptr_t RenderThread::main() {
    emugl::ChannelBuffer buf;
    if (!mChannel->readFromGuest(&buf)) {
        return 0;
    }
    if (buf.size() != 4) {
        DBG("Error in protocol, RenderThread failed to start @%p\n", this);
        return 0;
    }
    const auto flags = *reinterpret_cast<const uint32_t*>(buf.data());
    // |flags| used to mean something, now they're not used.
    (void)flags;

    ChannelStream stream(mChannel, emugl::ChannelBuffer::kSmallSize);
    RenderThreadInfo tInfo;
    ChecksumCalculatorThreadInfo tChecksumInfo;
    ChecksumCalculator& checksumCalc = tChecksumInfo.get();
    ReadBuffer readBuf(&stream);

    //
    // initialize decoders
    //
    tInfo.m_glDec.initGL(gles1_dispatch_get_proc_func, NULL);
    tInfo.m_gl2Dec.initGL(gles2_dispatch_get_proc_func, NULL);
    initRenderControlContext(&tInfo.m_rcDec);

    bool progress;
    do {
        progress = false;
        // try to process some of the command buffer using the GLESv1
        // decoder
        //
        // DRIVER WORKAROUND:
        // On Linux with NVIDIA GPU's at least, we need to avoid performing
        // GLES ops while someone else holds the FrameBuffer write lock.
        //
        // To be more specific, on Linux with NVIDIA Quadro K2200 v361.xx,
        // we get a segfault in the NVIDIA driver when glTexSubImage2D
        // is called at the same time as glXMake(Context)Current.
        //
        // To fix, this driver workaround avoids calling
        // any sort of GLES call when we are creating/destroying EGL
        // contexts.
        //FrameBuffer::getFB()->lockContextStructureRead();
        progress |= tInfo.m_glDec.decode(&readBuf, &stream, &checksumCalc);

        //
        // try to process some of the command buffer using the GLESv2
        // decoder
        //
        progress |= tInfo.m_gl2Dec.decode(&readBuf, &stream, &checksumCalc);
        //FrameBuffer::getFB()->unlockContextStructureRead();

        //
        // try to process some of the command buffer using the
        // renderControl decoder
        //
        progress |= tInfo.m_rcDec.decode(&readBuf, &stream, &checksumCalc);
    }
    while (progress);

    // exit sync thread, if any.
    SyncThread::destroySyncThread();

    //
    // Release references to the current thread's context/surfaces if any
    //
    FrameBuffer::getFB()->bindContext(0, 0, 0);
    if (tInfo.currContext || tInfo.currDrawSurf || tInfo.currReadSurf) {
        fprintf(stderr,
                "ERROR: RenderThread exiting with current context/surfaces\n");
    }

    FrameBuffer::getFB()->drainWindowSurface();
    FrameBuffer::getFB()->drainRenderContext();

    DBG("Exited a RenderThread @%p\n", this);

    return 0;
}
