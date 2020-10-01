// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "android/emulation/AndroidAsyncMessagePipe.h"

#include <cassert>

namespace android {

AndroidAsyncMessagePipe::AndroidAsyncMessagePipe(AndroidPipe::Service* service,
                                                 PipeArgs&& args)
    : AndroidPipe(args.hwPipe, service),
      mHandle(args.handle),
      mDeleter(std::move(args.deleter)) {
    LOG(VERBOSE) << "Registering pipe service " << service->name();
}

AndroidAsyncMessagePipe::~AndroidAsyncMessagePipe() {
    mState->mDeleted = true;
}

void AndroidAsyncMessagePipe::send(std::vector<uint8_t>&& data) {
    std::lock_guard<std::recursive_mutex> lock(mState->mMutex);
    DLOG(VERBOSE) << "Sending packet, " << data.size() << " bytes";
    mOutgoingPackets.emplace_back(std::move(data));

    signalWake(PIPE_WAKE_READ);
}

void AndroidAsyncMessagePipe::queueCloseFromHost() {
    std::lock_guard<std::recursive_mutex> lock(mState->mMutex);
    mCloseQueued = true;
}

void AndroidAsyncMessagePipe::onSave(base::Stream* stream) {
    std::lock_guard<std::recursive_mutex> lock(mState->mMutex);
    if (mIncomingPacket) {
        stream->putByte(1);
        mIncomingPacket->serialize(stream);
    } else {
        stream->putByte(0);
    }

    const size_t outgoingPacketCount = mOutgoingPackets.size();
    stream->putBe64(outgoingPacketCount);

    for (const OutgoingPacket& packet : mOutgoingPackets) {
        packet.serialize(stream);
    }
}

void AndroidAsyncMessagePipe::onLoad(base::Stream* stream) {
    std::lock_guard<std::recursive_mutex> lock(mState->mMutex);
    mIncomingPacket.clear();
    mOutgoingPackets.clear();

    const bool hasIncomingPacket = stream->getByte() != 0;
    if (hasIncomingPacket) {
        mIncomingPacket = IncomingPacket(stream);
    }

    const uint64_t outgoingPacketCount = stream->getBe64();
    for (uint64_t i = 0; i < outgoingPacketCount; ++i) {
        mOutgoingPackets.emplace_back(stream);
    }
}

bool AndroidAsyncMessagePipe::allowRead() const {
    if (mCloseQueued) {
        return false;
    }

    // If we're partially done reading data, complete that first.
    if (mIncomingPacket && !mIncomingPacket->complete()) {
        return true;
    }

    // Otherwise, only allow read if we don't have anything to write.
    return mOutgoingPackets.empty();
}

int AndroidAsyncMessagePipe::readBuffers(const AndroidPipeBuffer* buffers,
                                         int numBuffers) {
    // Hold onto the state in case we get deleted during callbacks.
    std::shared_ptr<PipeState> state = mState;
    std::lock_guard<std::recursive_mutex> lock(state->mMutex);
    if (!allowRead()) {
        DLOG(VERBOSE) << "Returning PIPE_ERROR_AGAIN, incoming="
                      << (mIncomingPacket ? 1 : 0)
                      << ", outgoing=" << mOutgoingPackets.size();
        return PIPE_ERROR_AGAIN;
    }

    std::vector<IncomingPacket> completedPackets;

    int bytesRead = 0;
    for (int i = 0; i < numBuffers && allowRead(); ++i) {
        const AndroidPipeBuffer& buffer = buffers[i];

        // Note that we check allowRead() again here, if a packet gets sent from
        // the onMessage() callback we stop reading additional packets, in order
        // to ensure the responses are sent in-order.
        for (size_t bufferReadOffset = 0;
             bufferReadOffset != buffer.size && allowRead();) {
            if (!mIncomingPacket) {
                mIncomingPacket = IncomingPacket();
            }

            const size_t currentBytesRead = mIncomingPacket->copyFromBuffer(
                    buffers[i], &bufferReadOffset);
            assert(bytesRead + currentBytesRead < INT32_MAX);
            bytesRead += static_cast<int>(currentBytesRead);

            if (mIncomingPacket->complete()) {
                DLOG(VERBOSE) << "Packet complete, "
                              << mIncomingPacket->messageLength << " bytes";
                IncomingPacket packet = std::move(mIncomingPacket.value());
                mIncomingPacket.clear();

                // onMessage may delete this, it's not safe to access any
                // data members past this point.
                onMessage(packet.data);
                if (state->mDeleted) {
                    DLOG(VERBOSE) << "Pipe deleted during onMessage.";
                    return bytesRead;
                }
            }
        }
    }

    DLOG(VERBOSE) << "readBuffers complete, " << bytesRead << " bytes total";
    return bytesRead;
}

int AndroidAsyncMessagePipe::writeBuffers(AndroidPipeBuffer* buffers,
                                          int numBuffers) {
    // Hold onto the state in case we get deleted during callbacks.
    std::shared_ptr<PipeState> state = mState;
    std::lock_guard<std::recursive_mutex> lock(state->mMutex);
    if (mOutgoingPackets.empty()) {
        DLOG(VERBOSE) << "Returning PIPE_ERROR_AGAIN, outgoing="
                      << mOutgoingPackets.size();
        return PIPE_ERROR_AGAIN;
    }

    int bytesWritten = 0;
    for (int i = 0; i < numBuffers && !mOutgoingPackets.empty(); ++i) {
        size_t bufferWriteOffset = 0;
        AndroidPipeBuffer& buffer = buffers[i];

        while (!mOutgoingPackets.empty() && bufferWriteOffset != buffer.size) {
            OutgoingPacket& packet = mOutgoingPackets.front();

            packet.copyToBuffer(buffers[i], &bufferWriteOffset);

            if (packet.complete()) {
                DLOG(VERBOSE) << "Packet send complete, " << packet.data.size()
                              << " bytes";
                mOutgoingPackets.pop_front();
            }
        }

        assert(bytesWritten + bufferWriteOffset < INT32_MAX);
        bytesWritten += int(bufferWriteOffset);
    }

    if (mCloseQueued && mOutgoingPackets.empty()) {
        DLOG(VERBOSE) << "Closing pipe, queued close operation.";
        // It's not safe to use any members past this point.
        closeFromHost();
    }

    DLOG(VERBOSE) << "writeBuffers complete, " << bytesWritten << " bytes";
    return bytesWritten;
}

unsigned AndroidAsyncMessagePipe::onGuestPoll() const {
    std::lock_guard<std::recursive_mutex> lock(mState->mMutex);
    // Note that in/out is in the context of the guest, so it's "out" of the
    // guest OS.

    // If we're partially done reading data, complete that first.
    if (mIncomingPacket && !mIncomingPacket->complete()) {
        return PIPE_POLL_OUT;
    }

    return !mOutgoingPackets.empty() ? PIPE_POLL_IN : PIPE_POLL_OUT;
}

AndroidAsyncMessagePipe::OutgoingPacket::OutgoingPacket(
        std::vector<uint8_t>&& packetData)
    : data(std::move(packetData)) {
    CHECK(packetData.size() < size_t(UINT32_MAX))
            << "Message too large, " << data.size()
            << " should be < UINT32_MAX";
}

AndroidAsyncMessagePipe::OutgoingPacket::OutgoingPacket(base::Stream* stream) {
    offset = size_t(stream->getBe64());

    const std::string dataStr = stream->getString();
    data = std::vector<uint8_t>(dataStr.begin(), dataStr.end());
}

bool AndroidAsyncMessagePipe::OutgoingPacket::complete() const {
    return offset == length();
}

void AndroidAsyncMessagePipe::OutgoingPacket::copyToBuffer(
        AndroidPipeBuffer& buffer,
        size_t* writeOffset) {
    assert(*writeOffset < buffer.size);

    size_t currentWriteOffset = *writeOffset;
    size_t bytesAvailable = buffer.size - currentWriteOffset;

    // Copy the header, send messageLength in little-endian order.
    const uint32_t messageLength = static_cast<uint32_t>(data.size());
    while (bytesAvailable && offset < sizeof(messageLength)) {
        buffer.data[currentWriteOffset++] =
                (messageLength >> (offset++ * 8)) & 0xFF;
        --bytesAvailable;
    }

    // Copy the data.
    const size_t maxWrite = std::min(length() - offset, bytesAvailable);
    if (maxWrite > 0) {
        memcpy(buffer.data + currentWriteOffset,
               data.data() + offset - sizeof(messageLength), maxWrite);
        currentWriteOffset += maxWrite;
        offset += maxWrite;
    }

    DLOG(VERBOSE) << "copyToBuffer " << currentWriteOffset - *writeOffset
                  << " bytes";
    *writeOffset = currentWriteOffset;
}

void AndroidAsyncMessagePipe::OutgoingPacket::serialize(
        base::Stream* stream) const {
    stream->putBe64(offset);
    stream->putString(reinterpret_cast<const char*>(data.data()), data.size());
}

AndroidAsyncMessagePipe::IncomingPacket::IncomingPacket(base::Stream* stream) {
    headerBytesRead = size_t(stream->getBe64());
    messageLength = stream->getBe32();

    const std::string dataStr = stream->getString();
    data = std::vector<uint8_t>(dataStr.begin(), dataStr.end());
}

bool AndroidAsyncMessagePipe::IncomingPacket::lengthKnown() const {
    return headerBytesRead == sizeof(uint32_t);
}

base::Optional<size_t> AndroidAsyncMessagePipe::IncomingPacket::bytesRemaining()
        const {
    if (!lengthKnown()) {
        return {};
    }

    assert(data.size() <= size_t(messageLength));
    return messageLength - uint32_t(data.size());
}

bool AndroidAsyncMessagePipe::IncomingPacket::complete() const {
    auto remaining = bytesRemaining();
    return remaining && remaining.value() == 0;
}

size_t AndroidAsyncMessagePipe::IncomingPacket::copyFromBuffer(
        const AndroidPipeBuffer& buffer,
        size_t* readOffset) {
    size_t bytesRead = 0;
    size_t currentReadOffset = *readOffset;

    // Read the header first.
    while (currentReadOffset < buffer.size && !lengthKnown()) {
        // messageLength is read in little-endian order.
        messageLength |= uint32_t(buffer.data[currentReadOffset++])
                         << (headerBytesRead * 8);
        ++bytesRead;
        ++headerBytesRead;

        DLOG_IF(VERBOSE, lengthKnown())
                << "Packet length known: " << messageLength;
    }

    // If the length isn't known at this point, currentReadOffset must equal
    // buffer.size.  If not, bytesRemaining().value() will assert.
    if (currentReadOffset < buffer.size && !complete()) {
        const size_t maxRead = std::min(buffer.size - currentReadOffset,
                                        bytesRemaining().value());
        const uint8_t* readStart = buffer.data + currentReadOffset;
        data.insert(data.end(), readStart, readStart + maxRead);

        bytesRead += maxRead;
        currentReadOffset += maxRead;
    }

    DLOG(VERBOSE) << "copyFromBuffer " << currentReadOffset - *readOffset
                  << " bytes";
    *readOffset = currentReadOffset;

    return bytesRead;
}

void AndroidAsyncMessagePipe::IncomingPacket::serialize(
        base::Stream* stream) const {
    stream->putBe64(headerBytesRead);
    stream->putBe32(messageLength);
    stream->putString(reinterpret_cast<const char*>(data.data()), data.size());
}

class CallbackMessagePipeService : public AndroidAsyncMessagePipe {
public:
    class Service
        : public AndroidAsyncMessagePipe::Service<CallbackMessagePipeService> {
    public:
        Service(const char* serviceName,
                OnMessageCallbackFunction onMessageCallback)
            : AndroidAsyncMessagePipe::Service<CallbackMessagePipeService>(
                      serviceName),
              mOnMessageCallback(onMessageCallback) {}

        OnMessageCallbackFunction mOnMessageCallback;
    };

    CallbackMessagePipeService(AndroidPipe::Service* service,
                               PipeArgs&& pipeArgs)
        : AndroidAsyncMessagePipe(service, std::move(pipeArgs)),
          mService(static_cast<Service*>(service)) {
        AsyncMessagePipeHandle handle = getHandle();
        Service* simpleService = mService;
        mOnSend = [simpleService, handle](std::vector<uint8_t>&& data) {
            auto pipeRef = simpleService->getPipe(handle);
            if (pipeRef) {
                pipeRef.value().pipe->send(std::move(data));
            }
        };
    }

    void onMessage(const std::vector<uint8_t>& data) override {
        mService->mOnMessageCallback(data, mOnSend);
    }

private:
    Service* const mService;
    PipeSendFunction mOnSend;
};

void registerAsyncMessagePipeService(
        const char* serviceName,
        OnMessageCallbackFunction onMessageCallback) {
    registerAsyncMessagePipeService(
        std::make_unique<CallbackMessagePipeService::Service>(
            serviceName, onMessageCallback));
}

}  // namespace android
