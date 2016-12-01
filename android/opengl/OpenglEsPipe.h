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

#include "android/base/Compiler.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/android_pipe.h"

#include "OpenglRender/RenderChannel.h"

#include <memory>

namespace android {

// OpenglEsPipe - an implementation of the fast "opengles" pipe for the
// GPU emulation.
//
// This class provides a
class OpenglEsPipe {
public:
    using Ptr = std::shared_ptr<OpenglEsPipe>;

    ~OpenglEsPipe();

    // Registers the "opengles" pipe type.
    static void registerPipeType();

    // Performs a guest wake operation, based on the |flags| parameter.
    void wakeOperation(int flags);

private:
    OpenglEsPipe(void* mHwpipe);
    // 2-phase initialization because of the ban of exceptions. Grrr.
    bool initialize(Ptr self);

    static const AndroidPipeFuncs kPipeFuncs;

    // Pipe interface needed for kPipeFuncs.
    static OpenglEsPipe* create(void* mHwpipe);
    void onCloseByGuest();
    void onWakeOn(int flags);
    unsigned onPoll();
    int onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers);
    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers);

    // RenderChannel state change callback
    void onIoEvent(emugl::RenderChannel::State state,
                   emugl::RenderChannel::EventSource source);

    // Generic private functions
    bool canRead() const;
    bool canWrite() const;
    bool canReadAny() const;

    void close(bool lock);
    void processIoEvents(bool lock);
    int sendReadyStatus() const;

private:
    // Pipe is self-owned, so this is the pointer that keeps it alive. To delete
    // the pipe, just reset it.
    Ptr mThis;

    void* mHwpipe = nullptr;
    emugl::RenderChannelPtr mChannel;

    // Guest state tracking - if it requested us to wake on read/write
    // availability. If guest doesn't care about some operation type, we should
    // not wake it when that operation becomes available.
    bool mCareAboutRead = false;
    bool mCareAboutWrite = false;

    // Set to |true| if the pipe is in working state, |false| means we're not
    // initialized or the pipe is closed, so one may not call any functions
    // taking |mHwpipe| as an argument.
    bool mIsWorking = false;
    // Protects |mHwpipe| and |mIsWorking|.
    base::Lock mPipeWorkingLock;

    // These two variables serve as a reading buffer for the guest.
    // Each time we get a read request, first we extract a single chunk from
    // the |mChannel| into here, and then copy its content into the
    // guest-supplied memory.
    // If guest didn't have enough room for the whole buffer, we track the
    // number of remaining bytes in |mDataForReadingLeft| for the next read().
    emugl::ChannelBuffer mDataForReading;
    size_t mDataForReadingLeft = 0;

    DISALLOW_COPY_ASSIGN_AND_MOVE(OpenglEsPipe);
};

}  // namespace android
