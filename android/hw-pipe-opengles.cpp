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
#include "android/hw-pipe-opengles.h"

#include "android/base/Optional.h"
#include "android/emulation/VmLock.h"
#include "android/opengles.h"

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

namespace android {

const AndroidPipeFuncs OpenglEsPipe::kPipeFuncs = {
        [](void* hwpipe, void* data, const char* args) -> void* {
            return OpenglEsPipe::create(hwpipe);
        },
        [](void* pipe) { static_cast<OpenglEsPipe*>(pipe)->onCloseByGuest(); },
        [](void* pipe, const AndroidPipeBuffer* buffers, int numBuffers) {
            return static_cast<OpenglEsPipe*>(pipe)->onGuestSend(buffers,
                                                                 numBuffers);
        },
        [](void* pipe, AndroidPipeBuffer* buffers, int numBuffers) {
            return static_cast<OpenglEsPipe*>(pipe)->onGuestRecv(buffers,
                                                                 numBuffers);
        },
        [](void* pipe) { return static_cast<OpenglEsPipe*>(pipe)->onPoll(); },
        [](void* pipe, int flags) {
            return static_cast<OpenglEsPipe*>(pipe)->onWakeOn(flags);
        },
        nullptr, // we can't save these
        nullptr, // we can't load these
};

void OpenglEsPipe::registerPipeType() {
    android_pipe_add_type("opengles", nullptr, &OpenglEsPipe::kPipeFuncs);
}

void OpenglEsPipe::processIoEvents(bool lock) {
    int wakeFlags = 0;

    if (careAboutRead && canReadAny()) {
        wakeFlags |= PIPE_WAKE_READ;
        careAboutRead = false;
    }
    if (careAboutWrite && canWrite()) {
        wakeFlags |= PIPE_WAKE_WRITE;
        careAboutWrite = false;
    }

    /* Send wake signal to the guest if needed */
    if (wakeFlags != 0) {
        android::base::Optional<android::base::ScopedVmLock> vmLock;
        if (lock) {
            vmLock.emplace();   // lock only if we need it
        }
        android_pipe_wake(hwpipe, wakeFlags);
    }
}

void OpenglEsPipe::onCloseByGuest() {
    D("%s", __func__);

    mIsWorking = false;
    channel->stop();
    // stop callback will call close() which deletes the pipe
}

void OpenglEsPipe::onWakeOn(int flags) {
    D("%s: flags=%d", __FUNCTION__, flags);

    careAboutRead |= (flags & PIPE_WAKE_READ) != 0;
    careAboutWrite |= (flags & PIPE_WAKE_WRITE) != 0;

    processIoEvents(false);
}

unsigned OpenglEsPipe::onPoll() {
    D("%s", __FUNCTION__);

    unsigned ret = 0;
    if (canReadAny()) {
        ret |= PIPE_POLL_IN;
    }
    if (canWrite()) {
        ret |= PIPE_POLL_OUT;
    }
    return ret;
}

int OpenglEsPipe::sendReadyStatus() const {
    if (mIsWorking) {
        if (canWrite()) {
            return 0;
        }
        return PIPE_ERROR_AGAIN;
    } else if (!hwpipe) {
        return PIPE_ERROR_IO;
    } else {
        return PIPE_ERROR_INVAL;
    }
}

int OpenglEsPipe::onGuestSend(const AndroidPipeBuffer* buffers,
                              int numBuffers) {
    D("%s", __FUNCTION__);

    if (int ret = sendReadyStatus()) {
        return ret;
    }

    const AndroidPipeBuffer* const buffEnd = buffers + numBuffers;
    int count = 0;
    for (auto buff = buffers; buff < buffEnd; buff++) {
        count += buff->size;
    }

    emugl::ChannelBuffer outBuffer;
    outBuffer.reserve(count);
    for (auto buff = buffers; buff < buffEnd; buff++) {
        outBuffer.insert(outBuffer.end(), buff->data, buff->data + buff->size);
    }

    if (!channel->write(std::move(outBuffer))) {
        return PIPE_ERROR_IO;
    }

    return count;
}

int OpenglEsPipe::onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) {
    D("%s", __FUNCTION__);

    // Consume the pipe's dataForReading, then put the next received data piece
    // there. repeat until the buffers are full.
    int len = 0;
    size_t buffOffset = 0;

    auto buff = buffers;
    const auto buffEnd = buff + numBuffers;
    while (buff != buffEnd) {
        if (dataForReadingLeft == 0) {
            // No data left, read a new chunk from the channel.
            //
            // Spin a little bit here: many GL calls are much faster than
            // the whole host-to-guest-to-host transition.
            int spinCount = 50;
            while (!channel->read(&dataForReading, false)) {
                if (channel->isStopped()) {
                    return PIPE_ERROR_IO;
                } else if (len == 0) {
                    if (canRead()) {
                        continue;
                    }
                    return PIPE_ERROR_AGAIN;
                } else {
                    return len;
                }
            }

            dataForReadingLeft = dataForReading.size();
        }

        const size_t curSize =
                std::min(buff->size - buffOffset, dataForReadingLeft);
        memcpy(buff->data + buffOffset,
               dataForReading.data() +
                       (dataForReading.size() - dataForReadingLeft),
               curSize);

        len += curSize;
        dataForReadingLeft -= curSize;
        buffOffset += curSize;
        if (buffOffset == buff->size) {
            ++buff;
            buffOffset = 0;
        }
    }

    return len;
}

OpenglEsPipe::~OpenglEsPipe() {
    D("%s", __FUNCTION__);
}

OpenglEsPipe* OpenglEsPipe::create(void* hwpipe) {
    D("%s", __FUNCTION__);

    if (!android_getOpenglesRenderer()) {
        // This should never happen, unless there is a bug in the
        // emulator's initialization, or the system image.
        D("Trying to open the OpenGLES pipe without GPU emulation!");
        return nullptr;
    }

    std::unique_ptr<OpenglEsPipe> pipe(new OpenglEsPipe(hwpipe));
    if (!pipe) {
        D("Out of memory when creating an OpenGLES pipe!");
        return nullptr;
    }

    if (!pipe->initialize()) {
        return nullptr;
    }

    return pipe.release();
}

OpenglEsPipe::OpenglEsPipe(void* hwpipe) : hwpipe(hwpipe) {
    D("%s", __FUNCTION__);

    assert(hwpipe != nullptr);
}

bool OpenglEsPipe::initialize() {
    D("%s", __FUNCTION__);

    channel = android_getOpenglesRenderer()->createRenderChannel();
    if (!channel) {
        D("Failed to create an OpenGLES pipe channel!");
        return false;
    }

    // Update the state before setting the callback, as the callback is called
    // at once.
    mIsWorking = true;

    channel->setEventCallback([this](emugl::RenderChannel::State state,
                                     emugl::RenderChannel::EventSource source) {
        this->onIoEvent(state, source);
    });

    return true;
}

void OpenglEsPipe::onIoEvent(emugl::RenderChannel::State state,
                             emugl::RenderChannel::EventSource source) {
    D("%s: %d/%d", __FUNCTION__, (int)state, (int)source);

    using State = emugl::RenderChannel::State;

    // We have to lock the qemu global mutex if this event isn't coming
    // from the pipe - because it means we're currently running on a
    // RenderThread.
    const bool needsLock =
            (source != emugl::RenderChannel::EventSource::Client);

    if ((state & State::Stopped) != State::Empty) {
        close(needsLock);
        return;
    }

    processIoEvents(needsLock);
}

bool OpenglEsPipe::canRead() const {
    return (channel->currentState() & emugl::RenderChannel::State::CanRead) !=
           emugl::RenderChannel::State::Empty;
}

bool OpenglEsPipe::canWrite() const {
    return (channel->currentState() & emugl::RenderChannel::State::CanWrite) !=
           emugl::RenderChannel::State::Empty;
}

void OpenglEsPipe::close(bool lock) {
    D("%s", __FUNCTION__);

    // If the pipe isn't in a working state, delete immediately
    if (!mIsWorking) {
        channel->stop();
        delete this;
        return;
    }

    // Force the closure of the channel - if a guest is blocked
    // waiting for a wake signal, it will receive an error.
    if (hwpipe) {
        android::base::Optional<android::base::ScopedVmLock> vmLock;
        if (lock) {
            vmLock.emplace();   // lock only if we need it
        }
        android_pipe_close(hwpipe);
        hwpipe = nullptr;
    }

    mIsWorking = false;
}

}  // namespace android
