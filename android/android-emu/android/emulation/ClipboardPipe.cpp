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

#include "android/emulation/ClipboardPipe.h"

#include "android/base/memory/LazyInstance.h"
#include "android/clipboard-pipe.h"

#include <cassert>

namespace android {
namespace emulation {

static constexpr auto emptyCallback = [](const uint8_t*, size_t) {};

// A storage for the current clipboard pipe instance data.
// Shared pointer is required as there are potential races between clipboard
// data changing from the host and guest's request to close the pipe. We need
// to keep the object alive if we've started some operation.
struct ClipboardPipeInstance {
    android::base::Lock pipeLock;
    ClipboardPipe::Ptr pipe;

    ClipboardPipe::GuestClipboardCallback guestClipboardCallback =
            emptyCallback;
};

static android::base::LazyInstance<ClipboardPipeInstance> sInstance = {};

ClipboardPipe::ClipboardPipe(void* hwPipe, Service* svc)
    : AndroidPipe(hwPipe, svc) {}

void ClipboardPipe::onGuestClose() {
    ClipboardPipe::Service::closePipe();
}

unsigned ClipboardPipe::onGuestPoll() const {
    unsigned result = PIPE_POLL_OUT;
    if (mGuestWriteState.hasData()) {
        result |= PIPE_POLL_IN;
    }
    return result;
}

int ClipboardPipe::processOperation(OperationType operation,
                                    ReadWriteState* state,
                                    const AndroidPipeBuffer* pipeBuffers,
                                    int numPipeBuffers) {
    const AndroidPipeBuffer* pipeBuf = pipeBuffers;
    const AndroidPipeBuffer* const endPipeBuf = pipeBuf + numPipeBuffers;
    int pipeBufOffset = 0;
    int totalProcessed = 0;

    while (pipeBuf != endPipeBuf) {
        // Decide how many bytes need to be read/written during the current
        // iteration.
        auto iterBytes = std::min(
                state->size(), static_cast<int>(pipeBuf->size) - pipeBufOffset);
        if (iterBytes == 0) {
            // Looks like we've finished early on the host. That's OK as guest
            // could pass a fixed-size buffer instead of using the exact size
            // we gave it.
            break;
        }

        const auto pipeBufPos = pipeBuf->data + pipeBufOffset;
        if (operation == OperationType::WriteToGuest) {
            // const_cast is ok here since AndroidPipeBuffer passed into the
            // function is spefically for writing.
            memcpy(const_cast<uint8_t*>(pipeBufPos), state->data(), iterBytes);
        } else {  // opType == OperationType::ReadFromGuest
            memcpy(state->data(), pipeBufPos, iterBytes);
        }
        totalProcessed += iterBytes;
        pipeBufOffset += iterBytes;
        if (pipeBufOffset >= static_cast<int>(pipeBuf->size)) {
            ++pipeBuf;
            pipeBufOffset = 0;
        }

        state->processedBytes += iterBytes;
        if (state->size() == 0 && !state->dataSizeTransferred) {
            // Done with size transfer, switch to the actual contents.
            state->dataSizeTransferred = true;
            state->processedBytes = 0;
            if (operation == OperationType::ReadFromGuest) {
                // If we're reading from the guest clipboard, make sure the
                // buffer on our side has enough space.
                state->buffer.resize(state->dataSize);
            }
        }
    }
    return totalProcessed;
}

int ClipboardPipe::onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) {
    if (ReadWriteState* stateWithData = mGuestWriteState.pickStateWithData()) {
        return processOperation(OperationType::WriteToGuest, stateWithData,
                                buffers, numBuffers);
    }
    return PIPE_ERROR_AGAIN;
}

int ClipboardPipe::onGuestSend(const AndroidPipeBuffer* buffers,
                               int numBuffers) {
    int result = processOperation(OperationType::ReadFromGuest,
                                  &mGuestReadState, buffers, numBuffers);
    if (mGuestReadState.isFinished()) {
        sInstance->guestClipboardCallback(mGuestReadState.buffer.data(),
                                          mGuestReadState.processedBytes);
        mGuestReadState.reset();
    }
    return result;
}

void registerClipboardPipeService() {
    android::AndroidPipe::Service::add(new ClipboardPipe::Service());
}

void ClipboardPipe::setGuestClipboardCallback(
        ClipboardPipe::GuestClipboardCallback cb) {
    sInstance->guestClipboardCallback = cb ? std::move(cb) : emptyCallback;
}

void ClipboardPipe::wakeGuestIfNeeded() {
    if (mWakeOnRead && mGuestWriteState.hasData()) {
        signalWake(PIPE_WAKE_READ);
        mWakeOnRead = false;
    }
}

void ClipboardPipe::setGuestClipboardContents(const uint8_t* buf, size_t len) {
    mGuestWriteState.queueContents(buf, len);
    wakeGuestIfNeeded();
}

////////////////////////////////////////////////////////////////////////////////

ClipboardPipe::Service::Service() : AndroidPipe::Service("clipboard") {}

AndroidPipe* ClipboardPipe::Service::create(void* hwPipe, const char* args) {
    const auto pipe = new ClipboardPipe(hwPipe, this);
    {
        android::base::AutoLock lock(sInstance->pipeLock);
        assert(!sInstance->pipe);
        sInstance->pipe.reset(pipe);
    }
    return pipe;
}

ClipboardPipe::Ptr ClipboardPipe::Service::getPipe() {
    android::base::AutoLock lock(sInstance->pipeLock);
    return sInstance->pipe;
}

void ClipboardPipe::Service::closePipe() {
    android::base::AutoLock lock(sInstance->pipeLock);
    sInstance->pipe.reset();
}

////////////////////////////////////////////////////////////////////////////////

ClipboardPipe::WritingState::WritingState()
    : inProgress(&states[0]), queued(&states[1]) {}

void ClipboardPipe::WritingState::queueContents(const uint8_t* buf,
                                                size_t len) {
    base::AutoLock l(lock);
    queued->buffer.assign(buf, buf + len);
    queued->dataSizeTransferred = false;
    queued->dataSize = len;
    queued->processedBytes = 0;
}

ClipboardPipe::ReadWriteState*
ClipboardPipe::WritingState::pickStateWithData() {
    base::AutoLock l(lock);
    if (!inProgress->isFinished()) {
        return inProgress;
    }
    if (!queued->isFinished()) {
        std::swap(inProgress, queued);
        return inProgress;
    }
    return nullptr;
}

bool ClipboardPipe::WritingState::hasData() const {
    base::AutoLock l(lock);
    return !inProgress->isFinished() || !queued->isFinished();
}

////////////////////////////////////////////////////////////////////////////////

ClipboardPipe::ReadWriteState::ReadWriteState() {
    reset();
}

uint8_t* ClipboardPipe::ReadWriteState::data() {
    return (dataSizeTransferred ? buffer.data()
                                : reinterpret_cast<uint8_t*>(&dataSize)) +
           processedBytes;
}

int ClipboardPipe::ReadWriteState::size() const {
    return (dataSizeTransferred ? static_cast<int>(buffer.size())
                                : sizeof(dataSize)) -
           processedBytes;
}

void ClipboardPipe::ReadWriteState::reset() {
    dataSize = 0;
    processedBytes = 0;
    dataSizeTransferred = false;
    buffer.clear();
}

bool ClipboardPipe::ReadWriteState::isFinished() const {
    return dataSizeTransferred && processedBytes == dataSize;
}

}  // namespace emulation
}  // namespace android

void android_init_clipboard_pipe(void) {
    android::emulation::registerClipboardPipeService();
}
