// Copyright 2017 The Android Open Source Project
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

#include "android/emulation/AndroidPipe.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

// This is a utility class that can help implement simple
// request/reponse type of remote calls between guest and host
// guest side:
// send the request Message
//
// host side:
// handle the reqeust message, generate response message
// send it back;
//
//  Message {
//      int32_t length_of_payload;
//      uint8_t payload[1];
//  };
//
// examples are: gatekeeper/keymaster
//
// how to use it:
// 1. provide a function that read request payload, generate response payload
//
// VERY IMPORTANT: this function runs on a QEMU's CPU thread while BQL is
// locked. This means the function _must_ be quick and may not rely on anything
// that might need to also lock the BQL (e.g. main QEMU loop, or other pipe
// services)
//
// void MyXYZDecodeAndExecute(const std::vector<uint8_t> &input,
// std::vector<uint8_t> *output) {
//    // do stuff
//}
// 2. register the service
// void registerMyXYZService() {
//    android::AndroidPipe::Service::add(new
//    android::AndroidMessagePipe::Service("MyXYZService",
//                                      MyXYZDecodeAndExecute));
//}

namespace android {

class AndroidMessagePipe : public AndroidPipe {
public:
    // message header length: 4 bytes int; guest should put
    // the payload length into int and send it over
    // to host
    static constexpr int kMessageHeaderNumBytes = 4;

    using DataBuffer = std::vector<uint8_t>;
    using DecodeAndExecuteFunction =
            std::function<void(const DataBuffer&, DataBuffer*)>;

    //////////////////////////////////////////////////////////////////////////
    // The pipe service class for this implementation.
    class Service : public AndroidPipe::Service {
    public:
        Service(const char* serviceName, DecodeAndExecuteFunction cb)
            : AndroidPipe::Service(serviceName), m_cb(cb) {}

        bool canLoad() const override { return true; }

        virtual AndroidPipe* load(void* hwPipe,
                          const char* args,
                          base::Stream* stream) override {
            return new AndroidMessagePipe(hwPipe, this, m_cb, stream);
        }

        virtual AndroidPipe* create(void* mHwPipe, const char* args) override {
            return new AndroidMessagePipe(mHwPipe, this, m_cb, nullptr);
        }

    private:
        DecodeAndExecuteFunction m_cb;
    };

    AndroidMessagePipe(void* hwPipe,
                       android::AndroidPipe::Service* service,
                       DecodeAndExecuteFunction cb,
                       base::Stream* loadStream = nullptr);

    void onSave(base::Stream* stream) override;

    void onGuestClose(PipeCloseReason reason) override {
        delete this;
    }
    unsigned onGuestPoll() const override;
    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override;
    int onGuestSend(const AndroidPipeBuffer* buffers, int numBuffers) override;
    void onGuestWantWakeOn(int flags) override {}
protected:
    void resetRecvPayload(DataBuffer&& buffer);

private:
    void resetToInitState();

    void loadFromStream(base::Stream* stream);

    void switchGuestWaitingState();

    size_t copyData(uint8_t** bufdatap, size_t* bufsizep, bool fromBufdata);

    bool readyForGuestToReceive() const;

    bool readyForGuestToSend() const;

    void setOffset(uint32_t myoffset);

    uint32_t getOffset() const;

    enum class GuestWaitingState {
        SendMesgLength,
        SendMesgPayload,
        RecvMesgLength,
        RecvMesgPayload,
    };
    GuestWaitingState mGuestWaitingState = GuestWaitingState::SendMesgLength;

    int32_t mSendLengthBuffer;
    DataBuffer mSendPayloadBuffer;
    int32_t mRecvLengthBuffer;
    DataBuffer mRecvPayloadBuffer;

    int32_t m_expected = 0;
    uint8_t* m_curPos;
    DecodeAndExecuteFunction m_cb;
};

void registerAndroidMessagePipeService(
        const char* serviceName,
        android::AndroidMessagePipe::DecodeAndExecuteFunction cb);

}  // namespace android
