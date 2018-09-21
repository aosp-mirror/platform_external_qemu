// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/emulation/AndroidAsyncMessagePipe.h"

#include <cassert>

namespace android {

// Message header length, containing the size of the following message.
static constexpr size_t kMessageHeaderBytes = 4;

AndroidAsyncMessagePipe::AndroidAsyncMessagePipe(AndroidPipe::Service* service,
                                                 PipeArgs&& args)
    : AndroidPipe(args.hwPipe, service),
      mHandle(args.handle), mDeleter(std::move(args.deleter)) {
    LOG(INFO) << "Registering pipe service " << service->name();

    if (args.loadStream) {
        LOG(VERBOSE) << "Loading state from snapshot";
        onLoad(args.loadStream);
    }
}

void AndroidAsyncMessagePipe::send(const std::vector<uint8_t>& data) {
    base::AutoLock lock(mLock);
    mOutgoingPackets.emplace_back(data);

    signalWake(PIPE_WAKE_READ);
}

void AndroidAsyncMessagePipe::onSave(base::Stream* stream) {
    base::AutoLock lock(mLock);
    if (mIncomingPacket) {
        stream->putByte(1);
        stream->putBe64(mIncomingPacket->headerBytesRead);
        for (auto b : mIncomingPacket->messageLength) {
            stream->putByte(b);
        }

        stream->putString(
                reinterpret_cast<const char*>(mIncomingPacket->data.data()),
                mIncomingPacket->data.size());
    } else {
        stream->putByte(0);
    }

    const size_t outgoingPacketCount = mOutgoingPackets.size();
    stream->putBe64(outgoingPacketCount);

    for (const OutgoingPacket& packet : mOutgoingPackets) {
        stream->putBe64(packet.offset);
        for (auto b : packet.messageLength) {
            stream->putByte(b);
        }

        stream->putString(reinterpret_cast<const char*>(packet.data.data()),
                          packet.data.size());
    }
}

void AndroidAsyncMessagePipe::onLoad(base::Stream* stream) {
    base::AutoLock lock(mLock);
    mIncomingPacket.clear();
    mOutgoingPackets.clear();

    const bool hasIncomingPacket = stream->getByte() != 0;
    if (hasIncomingPacket) {
        mIncomingPacket = IncomingPacket();
        mIncomingPacket->headerBytesRead = size_t(stream->getBe64());
        for (uint8_t& b : mIncomingPacket->messageLength) {
            b = stream->getByte();
        }

        const std::string data = stream->getString();
        mIncomingPacket->data = std::vector<uint8_t>(data.begin(), data.end());
    }

    const uint64_t outgoingPacketCount = stream->getBe64();
    for (uint64_t i = 0; i < outgoingPacketCount; ++i) {
        const size_t offset = size_t(stream->getBe64());
        std::array<uint8_t, 4> messageLength;
        static_assert(sizeof(messageLength) ==
                              sizeof(decltype(OutgoingPacket::messageLength)),
                      "Message length out of sync");

        for (uint8_t& b : messageLength) {
            b = stream->getByte();
        }

        const std::string dataStr = stream->getString();
        std::vector<uint8_t> data(dataStr.begin(), dataStr.end());

        OutgoingPacket packet = OutgoingPacket(std::move(data));
        packet.offset = offset;
        packet.messageLength = messageLength;
        mOutgoingPackets.emplace_back(std::move(packet));
    }
}

bool AndroidAsyncMessagePipe::allowRead() const {
    // If we're partially done reading data, complete that first.
    if (mIncomingPacket && !mIncomingPacket->complete()) {
        return true;
    }

    // Otherwise, only allow read if we don't have anything to write.
    return mOutgoingPackets.empty();
}

int AndroidAsyncMessagePipe::readBuffers(const AndroidPipeBuffer* buffers,
                                         int numBuffers) {
    base::AutoLock lock(mLock);
    if (!allowRead()) {
        return PIPE_ERROR_AGAIN;
    }

    int bytesRead = 0;
    for (int i = 0; i < numBuffers && allowRead(); ++i) {
        if (!mIncomingPacket) {
            mIncomingPacket = IncomingPacket();
        }

        size_t bufferReadOffset = 0;
        const size_t currentBytesRead = mIncomingPacket->copyFromBuffer(buffers[i], &bufferReadOffset);
        assert(bytesRead + currentBytesRead < INT32_MAX);
        bytesRead += static_cast<int>(currentBytesRead);

        if (mIncomingPacket->complete()) {
            onMessage(mIncomingPacket->data);
            mIncomingPacket.clear();
        }
    }

    DLOG(VERBOSE) << "Received " << bytesRead << " bytes";
    return bytesRead;
}

int AndroidAsyncMessagePipe::writeBuffers(AndroidPipeBuffer* buffers,
                                          int numBuffers) {
    base::AutoLock lock(mLock);
    if (mOutgoingPackets.empty()) {
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
                mOutgoingPackets.pop_front();
            }
        }

        assert(bytesWritten + bufferWriteOffset < INT32_MAX);
        bytesWritten += int(bufferWriteOffset);
    }

    DLOG(VERBOSE) << "Sent " << bytesWritten << " bytes";
    return bytesWritten;
}

unsigned AndroidAsyncMessagePipe::onGuestPoll() const {
    base::AutoLock lock(mLock);
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
            << "Message too large, " << packetData.size()
            << " should be < UINT32_MAX";

    const uint32_t len = static_cast<uint32_t>(packetData.size());
    messageLength = {uint8_t(len >> 24), uint8_t(len >> 16), uint8_t(len >> 8),
                     uint8_t(len)};
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

    // Copy the header.
    while (bytesAvailable && offset < messageLength.size()) {
        buffer.data[currentWriteOffset++] = messageLength[offset++];
        --bytesAvailable;
    }

    // Copy the data.
    const size_t maxWrite = std::min(length() - offset, bytesAvailable);
    if (maxWrite > 0) {
        memcpy(buffer.data + currentWriteOffset,
               data.data() + offset - messageLength.size(), maxWrite);
        writeOffset += maxWrite;
    }

    DLOG(VERBOSE) << "Wrote " << currentWriteOffset - *writeOffset << " bytes";
    *writeOffset = currentWriteOffset;
}

bool AndroidAsyncMessagePipe::IncomingPacket::lengthKnown() const {
    return headerBytesRead == kMessageHeaderBytes;
}

base::Optional<size_t> AndroidAsyncMessagePipe::IncomingPacket::bytesRemaining()
        const {
    if (!lengthKnown()) {
        return {};
    }

    const size_t length =
            size_t(messageLength[0] << 24) | size_t(messageLength[1] << 16) |
            size_t(messageLength[2] << 8) | size_t(messageLength[3]);
    assert(data.size() <= size_t(length));
    return length - uint32_t(data.size());
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
    if (!lengthKnown()) {
        const size_t maxRead = std::min(buffer.size - currentReadOffset,
                                        kMessageHeaderBytes - headerBytesRead);
        memcpy(messageLength.data(), buffer.data + currentReadOffset, maxRead);

        bytesRead += maxRead;
        currentReadOffset += maxRead;
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

    DLOG(VERBOSE) << "Read " << currentReadOffset - *readOffset << " bytes";
    *readOffset = currentReadOffset;

    return bytesRead;
}

}  // namespace android
