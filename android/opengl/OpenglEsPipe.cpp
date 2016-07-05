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

#include "android/base/memory/LazyInstance.h"
#include "android/base/Optional.h"
#include "android/base/synchronization/Lock.h"
#include "android/emulation/VmLock.h"
#include "android/opengles.h"
#include "android/opengles-pipe.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <unordered_map>

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

using android::base::Optional;
using android::ScopedVmLock;
using emugl::RenderChannel;

namespace android {

namespace {

// PipeWaker - a class to run Android Pipe operations in a correct manner.
//
// Background:
// Qemu1 and Qemu2 have completely different threading models:
// - Qemu1 runs all CPU-related operations and all timers/io/... events on its
//   main loop with no locking
// - Qemu2 has dedicated CPU threads to run guest read/write operations and a
//   "main loop" thread to run timers, check for pending IO etc. To make this
//   all work it has a global lock that has to be held during every qemu-related
//   function call.
//
// OpenglEsPipe is a just a set of callbacks running either on one of the CPU
// threads in Qemu2, or on the main loop in Qemu1, or on some render thread
// which performs the actual GPU emulation.
//
// Qemu1 has no locking mechanisms and it *requires* you to call its functions
// from its main thread. Qemu2 has this global mutex you need to acquire before
// running those operations, and doesn't really care about the thread.
//
// OpenglEsPipe needs to call android_pipe_host_signal_wake() and, sometimes,
// android_pipe_host_close(); these functions set the pipe's IRQ, so they have
// to comply with all those threading/locking rules on where and how to run.
// To add more to the complexity, Qemu2's dynamic code recompilation (TCG) is
// non thread-safe in even scarrier manner: one may not call any related
// functions from a non-main loop thread, even when holding the lock.
//
// Implementation:
// PipeWaker constructor takes a Looper* parameter, and operates based on it:
// - if it is not null, it assumes one needs to run on the looper's main thread
//   and creates a timer callback that will run all wake/close pipe operations
//   when you call PipeWaker::wake()/close(); the operation is just scheduled
//   to run in the timer callback.
//
// - if the looper is null, PipeWaker runs the pipe operation in the thread that
//   calls PipeWaker::wake/close, but holds the Qemu global lock for the
//   duration of the operation.
class PipeWaker final {
public:
    // Wake the |hwPipe|, setting its wake flags to |flags|.
    // If |needsLock| is set, lock the global Qemu lock (VmLock) for the wake
    // call.
    void wake(void* hwPipe, int flags, bool needsLock);

    // Same as the former, but close the pipe instead of waking it.
    void close(void* hwPipe, bool needsLock);

    // create() creates a static object instance, get() returns it.
    // |looper| - if set to non-null, schedule all operations on this looper's
    //            main loop. Otherwise, run them on the calling thread.
    static void create(Looper* looper);
    static PipeWaker& get();

    ~PipeWaker();

private:
    PipeWaker(Looper* looper);

    void onWakeTimer();
    static void performPipeOperation(void* pipe, int flags);

private:
    static PipeWaker* sThis;

    // This has to have all bits set to make sure it contains and overrides
    // all other possible flags - we don't need to wake a pipe if it's closed.
    static const int kWakeFlagsClose = ~0;

    std::unordered_map<void*, int> mWakeMap;  // HwPipe* -> wake/close flags
    base::Lock mWakeMapLock;                  // protects the mWakeMap
    LoopTimer* mWakeTimer = nullptr;

    DISALLOW_COPY_ASSIGN_AND_MOVE(PipeWaker);
};

PipeWaker* PipeWaker::sThis = nullptr;

PipeWaker& PipeWaker::get() {
    assert(sThis);
    return *sThis;
}

void PipeWaker::create(Looper* looper) {
    assert(!sThis);
    sThis = new PipeWaker(looper);
}

PipeWaker::PipeWaker(Looper* looper) {
    if (looper) {
        mWakeTimer =
                loopTimer_new(looper,
                              [](void* that, LoopTimer*) {
                                  static_cast<PipeWaker*>(that)->onWakeTimer();
                              },
                              this);
        if (!mWakeTimer) {
            D("%s: Failed to create a loop timer, "
              "falling back to regular locking!", __func__);
        }
    }
}

PipeWaker::~PipeWaker() {
    if (mWakeTimer) {
        loopTimer_stop(mWakeTimer);
        loopTimer_free(mWakeTimer);
    }
}

void PipeWaker::performPipeOperation(void* pipe, int flags) {
    if (flags == kWakeFlagsClose) {
        android_pipe_host_close(pipe);
    } else {
        android_pipe_host_signal_wake(pipe, flags);
    }
}

void PipeWaker::wake(void* pipe, int flags, bool needsLock) {
    if (mWakeTimer) {
        base::AutoLock lock(mWakeMapLock);
        const bool startTimer = mWakeMap.empty();
        // 'close' flags have all bits set, so they override everything else
        mWakeMap[pipe] |= flags;
        lock.unlock();

        if (startTimer) {
            loopTimer_startAbsolute(mWakeTimer, 0);
        }
    } else {
        // Qemu global lock abstraction, see android/emulation/VmLock.h
        Optional<ScopedVmLock> vmLock;
        if (needsLock) {
            vmLock.emplace();  // lock only if we need it
        }
        performPipeOperation(pipe, flags);
    }
}

void PipeWaker::close(void* pipe, bool needsLock) {
    wake(pipe, kWakeFlagsClose, needsLock);
}

void PipeWaker::onWakeTimer() {
    assert(mWakeTimer);

    std::unordered_map<void*, int> wakeMap;
    base::AutoLock lock(mWakeMapLock);
    wakeMap.swap(mWakeMap);
    lock.unlock();

    for (const auto& pair : wakeMap) {
        performPipeOperation(pair.first, pair.second);
    }

    lock.lock();
    // Check if we need to rearm the timer if someone has added an event during
    // the processing.
    if (!mWakeMap.empty()) {
        lock.unlock();
        loopTimer_startAbsolute(mWakeTimer, 0);
    }
}

}  // namespace

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
        nullptr,  // we can't save these
        nullptr,  // we can't load these
};

void OpenglEsPipe::registerPipeType() {
    android_pipe_add_type("opengles", nullptr, &OpenglEsPipe::kPipeFuncs);
}

void OpenglEsPipe::processIoEvents(bool lock) {
    int wakeFlags = 0;

    if (mCareAboutRead && canReadAny()) {
        wakeFlags |= PIPE_WAKE_READ;
        mCareAboutRead = false;
    }
    if (mCareAboutWrite && canWrite()) {
        wakeFlags |= PIPE_WAKE_WRITE;
        mCareAboutWrite = false;
    }

    /* Send wake signal to the guest if needed */
    if (wakeFlags != 0) {
        PipeWaker::get().wake(mHwpipe, wakeFlags, lock);
    }
}

void OpenglEsPipe::onCloseByGuest() {
    D("%s", __func__);

    mIsWorking = false;
    mChannel->stop();
    // stop callback will call close() which deletes the pipe
}

void OpenglEsPipe::onWakeOn(int flags) {
    D("%s: flags=%d", __FUNCTION__, flags);

    // We reset these flags when we actually wake the pipe, so only
    // add to them here.
    mCareAboutRead |= (flags & PIPE_WAKE_READ) != 0;
    mCareAboutWrite |= (flags & PIPE_WAKE_WRITE) != 0;

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
    } else if (!mHwpipe) {
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

    if (!mChannel->write(std::move(outBuffer))) {
        return PIPE_ERROR_IO;
    }

    return count;
}

int OpenglEsPipe::onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) {
    D("%s", __FUNCTION__);

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
                    if (canRead() || --spinCount > 0) {
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
                std::min(buff->size - buffOffset, mDataForReadingLeft);
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

OpenglEsPipe::OpenglEsPipe(void* hwpipe) : mHwpipe(hwpipe) {
    D("%s", __FUNCTION__);

    assert(hwpipe != nullptr);
}

bool OpenglEsPipe::initialize() {
    D("%s", __FUNCTION__);

    mChannel = android_getOpenglesRenderer()->createRenderChannel();
    if (!mChannel) {
        D("Failed to create an OpenGLES pipe channel!");
        return false;
    }

    // Update the state before setting the callback, as the callback is called
    // at once.
    mIsWorking = true;

    mChannel->setEventCallback([this](RenderChannel::State state,
                                      RenderChannel::EventSource source) {
        this->onIoEvent(state, source);
    });

    return true;
}

void OpenglEsPipe::onIoEvent(RenderChannel::State state,
                             RenderChannel::EventSource source) {
    D("%s: %d/%d", __FUNCTION__, (int)state, (int)source);

    // We have to lock the qemu global mutex if this event isn't coming
    // from the pipe - because it means we're currently running on a
    // RenderThread.
    const bool needsLock = (source != RenderChannel::EventSource::Client);

    if ((state & RenderChannel::State::Stopped) !=
        RenderChannel::State::Empty) {
        close(needsLock);
        return;
    }

    processIoEvents(needsLock);
}

bool OpenglEsPipe::canRead() const {
    return (mChannel->currentState() & RenderChannel::State::CanRead) !=
           RenderChannel::State::Empty;
}

bool OpenglEsPipe::canWrite() const {
    return (mChannel->currentState() & RenderChannel::State::CanWrite) !=
           RenderChannel::State::Empty;
}

bool OpenglEsPipe::canReadAny() const {
    return mDataForReadingLeft != 0 || canRead();
}

void OpenglEsPipe::close(bool lock) {
    D("%s", __FUNCTION__);

    // If the pipe isn't in a working state, delete immediately.
    if (!mIsWorking) {
        mChannel->stop();
        delete this;
        return;
    }

    // Force the closure of the channel - if a guest is blocked
    // waiting for a wake signal, it will receive an error.
    if (mHwpipe) {
        PipeWaker::get().close(mHwpipe, lock);
        mHwpipe = nullptr;
    }

    mIsWorking = false;
}

}  // namespace android

void android_init_opengles_pipe(Looper* looper) {
    android::PipeWaker::create(looper);
    android::OpenglEsPipe::registerPipeType();
}
