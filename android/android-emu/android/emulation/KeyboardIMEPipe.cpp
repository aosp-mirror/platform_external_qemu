// Copyright 2019 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/KeyboardIMEPipe.h"

#include "android/base/memory/LazyInstance.h"
#include "android/android-keyboard-ime-pipe.h"

#include <cassert>

namespace android {
namespace emulation {

static void emptyCallback(const uint8_t*, size_t) {}

bool KeyboardIMEPipe::sEnabled = true;

// A storage for the current keyboard IME pipe instance data.
// Shared pointer is required as there are potential races between keyboard IME
// data changing from the host and guest's request to close the pipe. We need
// to keep the object alive if we've started some operation.
struct KeyboardIMEPipeInstance {
    android::base::Lock pipeLock;
    KeyboardIMEPipe::Ptr pipe;

    KeyboardIMEPipe::GuestKeyboardIMECallback guestKeyboardIMECallback =
            emptyCallback;
};

static android::base::LazyInstance<KeyboardIMEPipeInstance> sInstance = {};

KeyboardIMEPipe::KeyboardIMEPipe(void* hwPipe, Service* svc)
    : AndroidPipe(hwPipe, svc) {}

void KeyboardIMEPipe::onGuestClose(PipeCloseReason reason) {
    KeyboardIMEPipe::Service::closePipe();
}

unsigned KeyboardIMEPipe::onGuestPoll() const {
    unsigned result = PIPE_POLL_OUT;
    if (sEnabled && mGuestWriteState.hasData()) {
        result |= PIPE_POLL_IN;
    }
    return result;
}

int KeyboardIMEPipe::processOperation(OperationType operation,
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
            printf("Got data[%s]\n", state->data());
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

int KeyboardIMEPipe::onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) {
    if (!sEnabled) {
        // Make sure we don't start a new transfer, but don't interrupt the one
        // already in progress.
        mGuestWriteState.clearQueued();
    }

    if (ReadWriteState* stateWithData = mGuestWriteState.pickStateWithData()) {
        return processOperation(OperationType::WriteToGuest, stateWithData,
                                buffers, numBuffers);
    }
    return PIPE_ERROR_AGAIN;
}

int KeyboardIMEPipe::onGuestSend(const AndroidPipeBuffer* buffers,
                               int numBuffers) {
    if (!sEnabled) {
        // Fake out that we've processed the data, and don't do anything.
        int total = 0;
        for (int i = 0; i != numBuffers; ++i) {
            total += buffers[i].size;
        }
        return total;
    }

    int result = processOperation(OperationType::ReadFromGuest,
                                  &mGuestReadState, buffers, numBuffers);
    if (mGuestReadState.isFinished()) {
        sInstance->guestKeyboardIMECallback(mGuestReadState.buffer.data(),
                                          mGuestReadState.processedBytes);
        mGuestReadState.reset();
    }
    return result;
}

void KeyboardIMEPipe::setEnabled(bool enabled) {
    sEnabled = enabled;
}

void registerKeyboardIMEPipeService() {
    android::AndroidPipe::Service::add(new KeyboardIMEPipe::Service());
}

void KeyboardIMEPipe::setGuestKeyboardIMECallback(
        KeyboardIMEPipe::GuestKeyboardIMECallback cb) {
    sInstance->guestKeyboardIMECallback = cb ? std::move(cb) : emptyCallback;
}

void KeyboardIMEPipe::wakeGuestIfNeeded() {
    if (sEnabled && mWakeOnRead && mGuestWriteState.hasData()) {
        signalWake(PIPE_WAKE_READ);
        mWakeOnRead = false;
    }
}

void KeyboardIMEPipe::setText(const uint8_t* buf, size_t len) {
    if (!sEnabled) {
        return; // who cares.
    }
    mGuestWriteState.queueContents(buf, len);
    wakeGuestIfNeeded();
}

////////////////////////////////////////////////////////////////////////////////

KeyboardIMEPipe::Service::Service() : AndroidPipe::Service("keyboardime") {printf("keyboard started\n");}

AndroidPipe* KeyboardIMEPipe::Service::create(void* hwPipe, const char* args) {
    const auto pipe = new KeyboardIMEPipe(hwPipe, this);
    {
        android::base::AutoLock lock(sInstance->pipeLock);
        assert(!sInstance->pipe);
        sInstance->pipe.reset(pipe);
    }
    return pipe;
}

KeyboardIMEPipe::Ptr KeyboardIMEPipe::Service::getPipe() {
    android::base::AutoLock lock(sInstance->pipeLock);
    return sInstance->pipe;
}

void KeyboardIMEPipe::Service::closePipe() {
    android::base::AutoLock lock(sInstance->pipeLock);
    sInstance->pipe.reset();
}

////////////////////////////////////////////////////////////////////////////////

KeyboardIMEPipe::WritingState::WritingState()
    : inProgress(&states[0]), queued(&states[1]) {}

void KeyboardIMEPipe::WritingState::queueContents(const uint8_t* buf,
                                                size_t len) {
    base::AutoLock l(lock);
    queued->buffer.assign(buf, buf + len);
    queued->dataSizeTransferred = false;
    queued->dataSize = len;
    queued->processedBytes = 0;
}

KeyboardIMEPipe::ReadWriteState*
KeyboardIMEPipe::WritingState::pickStateWithData() {
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

void KeyboardIMEPipe::WritingState::clearQueued() {
    base::AutoLock l(lock);
    // These two operations together indicate that there's nothing to transfer,
    // so it won't get picked for sending.
    queued->reset();
    queued->dataSizeTransferred = true;
}

bool KeyboardIMEPipe::WritingState::hasData() const {
    base::AutoLock l(lock);
    return !inProgress->isFinished() || !queued->isFinished();
}

////////////////////////////////////////////////////////////////////////////////

KeyboardIMEPipe::ReadWriteState::ReadWriteState() {
    reset();
}

uint8_t* KeyboardIMEPipe::ReadWriteState::data() {
    return (dataSizeTransferred ? buffer.data()
                                : reinterpret_cast<uint8_t*>(&dataSize)) +
           processedBytes;
}

int KeyboardIMEPipe::ReadWriteState::size() const {
    return (dataSizeTransferred ? static_cast<int>(buffer.size())
                                : sizeof(dataSize)) -
           processedBytes;
}

void KeyboardIMEPipe::ReadWriteState::reset() {
    dataSize = 0;
    processedBytes = 0;
    dataSizeTransferred = false;
    buffer.clear();
}

bool KeyboardIMEPipe::ReadWriteState::isFinished() const {
    return dataSizeTransferred && processedBytes == dataSize;
}

}  // namespace emulation
}  // namespace android

void android_init_keyboard_ime_pipe(void) {
    android::emulation::registerKeyboardIMEPipeService();
}

