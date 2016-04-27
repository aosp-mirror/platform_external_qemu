// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#pragma once

#include "android/emulation/android_pipe.h"

#include "OpenglRender/RenderChannel.h"

namespace android {

// OpenglEsPipe - an implementation of the fast "opengles" pipe for the
// GPU emulation.
class OpenglEsPipe {
public:
    ~OpenglEsPipe();

    static void registerPipeType();

private:
    OpenglEsPipe(void* hwpipe);
    bool initialize();

    static const AndroidPipeFuncs kPipeFuncs;

private:
    // Pipe interface needed for kPipeFuncs.
    static OpenglEsPipe* create(void* hwpipe);
    void onCloseByGuest();
    void onWakeOn(int flags);
    unsigned onPoll();
    int onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers);
    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers);

    void onIoEvent(emugl::RenderChannel::State state,
                   emugl::RenderChannel::EventSource source);

private:
    bool canRead() const;
    bool canWrite() const;

    bool canReadAny() const { return canRead() || dataForReadingLeft > 0; }

    void close(bool lock);
    void processIoEvents(bool lock);
    int sendReadyStatus() const;

private:
    void* hwpipe = nullptr;
    bool careAboutRead = false;
    bool careAboutWrite = false;
    emugl::RenderChannelPtr channel;
    emugl::ChannelBuffer dataForReading;
    size_t dataForReadingLeft = 0;
    bool mIsWorking = false;
};

}  // namespace android
