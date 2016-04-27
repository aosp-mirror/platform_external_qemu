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
    OpenglEsPipe(void* mHwpipe);
    bool initialize();

    static const AndroidPipeFuncs kPipeFuncs;

private:
    // Pipe interface needed for kPipeFuncs.
    static OpenglEsPipe* create(void* mHwpipe);
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
    bool canReadAny() const;

    void close(bool lock);
    void processIoEvents(bool lock);
    int sendReadyStatus() const;

private:
    void* mHwpipe = nullptr;
    bool mCareAboutRead = false;
    bool mCareAboutWrite = false;
    bool mIsWorking = false;
    emugl::RenderChannelPtr mChannel;
    emugl::ChannelBuffer mDataForReading;
    size_t mDataForReadingLeft = 0;
};

}  // namespace android
