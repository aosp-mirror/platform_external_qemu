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
#include "android/opengl/OpenglEsPipe.h"

#include "android/base/async/Looper.h"
#include "android/opengles.h"
#include "android/opengles-pipe.h"
#include "android/opengl/GLProcessPipe.h"

#include <atomic>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

// Set to 1 or 2 for debug traces
//#define DEBUG 1

#if DEBUG >= 1
#define D(...) printf(__VA_ARGS__), printf("\n"), fflush(stdout)
#else
#define D(...) ((void)0)
#endif

#if DEBUG >= 2
#define DD(...) printf(__VA_ARGS__), printf("\n"), fflush(stdout)
#else
#define DD(...) ((void)0)
#endif

using emugl::RenderChannel;
using emugl::RenderChannelPtr;
using ChannelState = emugl::RenderChannel::State;

namespace android {
namespace opengl {

namespace {

class EmuglPipe : public AndroidPipe {
public:
    //////////////////////////////////////////////////////////////////////////
    // The pipe service class for this implementation.
    class Service : public AndroidPipe::Service {
    public:
        Service() : AndroidPipe::Service("opengles") {}

        // Create a new EmuglPipe instance.
        virtual AndroidPipe* create(void* mHwPipe, const char* args) override {
            auto renderer = android_getOpenglesRenderer();
            if (!renderer) {
                // This should never happen, unless there is a bug in the
                // emulator's initialization, or the system image.
                D("Trying to open the OpenGLES pipe without GPU emulation!");
                return nullptr;
            }

            EmuglPipe* pipe = new EmuglPipe(mHwPipe, this, renderer);
            if (!pipe->mIsWorking) {
                delete pipe;
                pipe = nullptr;
            }
            return pipe;
        }

        // Really cannot save/load these pipes' state.
        virtual bool canLoad() const override { return false; }
    };

    /////////////////////////////////////////////////////////////////////////
    // Constructor, check that |mIsWorking| is true after this call to verify
    // that everything went well.
    EmuglPipe(void* hwPipe,
              Service* service,
              const emugl::RendererPtr& renderer)
        : AndroidPipe(hwPipe, service) {
        mChannel = renderer->createRenderChannel();
        if (!mChannel) {
            D("Failed to create an OpenGLES pipe channel!");
            return;
        }

        mIsWorking = true;
        mChannel->setEventCallback([this](ChannelState state,
                                          RenderChannel::EventSource source) {
            this->onChannelIoEvent(state);
        });
    }

    //////////////////////////////////////////////////////////////////////////
    // Overriden AndroidPipe methods

    virtual void onGuestClose() override {
        D("%s", __func__);
        mIsWorking = false;
        mChannel->stop();
        // stop callback will call close() which deletes the pipe
    }

    virtual unsigned onGuestPoll() const override {
        DD("%s", __func__);

        unsigned ret = 0;
        ChannelState state = mChannel->currentState();
        if (canReadAny(state)) {
            ret |= PIPE_POLL_IN;
        }
        if (canWrite(state)) {
            ret |= PIPE_POLL_OUT;
        }
        return ret;
    }

    virtual int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers)
            override {
        DD("%s", __func__);

        // Consume the pipe's dataForReading, then put the next received data piece
        // there. Repeat until the buffers are full or we're out of data
        // in the channel.
        int len = 0;
        size_t buffOffset = 0;

        auto buff = buffers;
        const auto buffEnd = buff + numBuffers;
        while (buff != buffEnd) {
            if (mDataForReadingLeft == 0) {
                // No data left, read a new chunk from the channel.
                //
                // Spin a little bit here: many GL calls are much faster than
                // the whole host-to-guest-to-host transition.
                int spinCount = 20;
                while (!mChannel->read(&mDataForReading,
                                    RenderChannel::CallType::Nonblocking)) {
                    if (mChannel->isStopped()) {
                        return PIPE_ERROR_IO;
                    } else if (len == 0) {
                        if (canRead(mChannel->currentState()) || --spinCount > 0) {
                            continue;
                        }
                        return PIPE_ERROR_AGAIN;
                    } else {
                        return len;
                    }
                }

                mDataForReadingLeft = mDataForReading.size();
            }

            const size_t curSize =
                    std::min(buff->size - buffOffset, mDataForReadingLeft.load());
            memcpy(buff->data + buffOffset,
                mDataForReading.data() +
                        (mDataForReading.size() - mDataForReadingLeft),
                curSize);

            len += curSize;
            mDataForReadingLeft -= curSize;
            buffOffset += curSize;
            if (buffOffset == buff->size) {
                ++buff;
                buffOffset = 0;
            }
        }

        return len;
    }

    virtual int onGuestSend(const AndroidPipeBuffer* buffers,
                            int numBuffers) override {
        DD("%s", __func__);

        if (int ret = sendReadyStatus()) {
            return ret;
        }

        // Count the total bytes to send.
        int count = 0;
        for (int n = 0; n < numBuffers; ++n) {
            count += buffers[n].size;
        }

        // Copy everything into a single RenderChannel::Buffer.
        RenderChannel::Buffer outBuffer;
        outBuffer.resize_noinit(count);
        auto ptr = outBuffer.data();
        for (int n = 0; n < numBuffers; ++n) {
            memcpy(ptr, buffers[n].data, buffers[n].size);
            ptr += buffers[n].size;
        }

        // Send it through the channel.
        if (!mChannel->write(std::move(outBuffer))) {
            return PIPE_ERROR_IO;
        }

        return count;
    }

    virtual void onGuestWantWakeOn(int flags) override {
        DD("%s: flags=%d", __func__, flags);

        // We reset these flags when we actually wake the pipe, so only
        // add to them here.
        mCareAboutRead |= (flags & PIPE_WAKE_READ) != 0;
        mCareAboutWrite |= (flags & PIPE_WAKE_WRITE) != 0;

        ChannelState state = mChannel->currentState();
        processIoEvents(state);
    }

private:
    // Returns true iff there is data to read from the emugl channel.
    static bool canRead(ChannelState state) {
        return (state & ChannelState::CanRead) != ChannelState::Empty;
    }

    // Returns true iff the emugl channel can accept data.
    static bool canWrite(ChannelState state) {
        return (state & ChannelState::CanWrite) != ChannelState::Empty;
    }

    // Returns true iff there is data to read from the local cache or from
    // the emugl channel.
    bool canReadAny(ChannelState state) const {
        return mDataForReadingLeft != 0 || canRead(state);
    }

    // Check that the pipe is working and that the render channel can be
    // written to. Return 0 in case of success, and one PIPE_ERROR_XXX
    // value on error.
    int sendReadyStatus() const {
        if (mIsWorking) {
            ChannelState state = mChannel->currentState();
            if (canWrite(state)) {
                return 0;
            }
            return PIPE_ERROR_AGAIN;
        } else if (!mHwPipe) {
            return PIPE_ERROR_IO;
        } else {
            return PIPE_ERROR_INVAL;
        }
    }

    // Check the read/write state of the render channel and signal the pipe
    // if any condition meets mCareAboutRead or mCareAboutWrite.
    void processIoEvents(ChannelState state) {
        int wakeFlags = 0;

        if (mCareAboutRead && canReadAny(state)) {
            wakeFlags |= PIPE_WAKE_READ;
            mCareAboutRead = false;
        }
        if (mCareAboutWrite && canWrite(state)) {
            wakeFlags |= PIPE_WAKE_WRITE;
            mCareAboutWrite = false;
        }

        // Send wake signal to the guest if needed.
        if (wakeFlags != 0) {
            signalWake(wakeFlags);
        }
    }

    // Called when an i/o event occurs on the render channel
    void onChannelIoEvent(ChannelState state) {
        D("%s: %d", __func__, (int)state);

        if ((state & ChannelState::Stopped) != ChannelState::Empty) {
            close();
        } else {
            processIoEvents(state);
        }
    }

    // Close the pipe, this may be called from the host or guest side.
    void close() {
        D("%s", __func__);

        // If the pipe isn't in a working state, delete immediately.
        if (!mIsWorking) {
            mChannel->stop();
            delete this;
            return;
        }

        // Force the closure of the channel - if a guest is blocked
        // waiting for a wake signal, it will receive an error.
        if (mHwPipe) {
            closeFromHost();
            mHwPipe = nullptr;
        }

        mIsWorking = false;
    }

    // A RenderChannel pointer used for communication.
    RenderChannelPtr mChannel;

    // Guest state tracking - if it requested us to wake on read/write
    // availability. If guest doesn't care about some operation type, we should
    // not wake it when that operation becomes available.
    // Note: we need an atomic or operation, and atomic<bool> doesn't have it -
    //  that's why it is atomic<char> here.
    std::atomic<char> mCareAboutRead {false};
    std::atomic<char> mCareAboutWrite {false};

    // Set to |true| if the pipe is in working state, |false| means we're not
    // initialized or the pipe is closed.
    bool mIsWorking = false;

    // These two variables serve as a reading buffer for the guest.
    // Each time we get a read request, first we extract a single chunk from
    // the |mChannel| into here, and then copy its content into the
    // guest-supplied memory.
    // If guest didn't have enough room for the whole buffer, we track the
    // number of remaining bytes in |mDataForReadingLeft| for the next read().
    RenderChannel::Buffer mDataForReading;
    std::atomic<size_t> mDataForReadingLeft { 0 };

    DISALLOW_COPY_ASSIGN_AND_MOVE(EmuglPipe);
};

}  // namespace

void registerPipeService() {
    android::AndroidPipe::Service::add(new EmuglPipe::Service());
    registerGLProcessPipeService();
}

}  // namespace opengl
}  // namespace android

// Declared in android/opengles-pipe.h
void android_init_opengles_pipe() {
    android::opengl::registerPipeService();
}
