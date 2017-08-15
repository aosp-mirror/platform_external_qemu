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

#include <memory>
#include <vector>
#include <string>

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
//      uint64_t length_of_payload;
//      uint8_t payload[1];
//  };
//
// examples are: gatekeeper/keymaster
//
// how to use it:
// 1. provide a function that read request payload, generate response payload
// void MyXYZDecodeAndExecute(std::vector<uint8_t> &input, std::vector<uint8_t> *output) {
//    // do stuff
//}
// 2. register the service
// void registerMyXYZService() {
//    android::AndroidPipe::Service::add(new android::AndroidMessagePipe::Service("MyXYZService", MyXYZDecodeAndExecute));
//}


namespace android {

class AndroidMessagePipe : public AndroidPipe {
public:

    const static int MESSAGE_LENGTH_BUFFER_SIZE = 8;

    typedef void (*DecodeAndExecute)(std::vector<uint8_t> &input, std::vector<uint8_t> * output);

    //////////////////////////////////////////////////////////////////////////
    // The pipe service class for this implementation.
    class Service : public AndroidPipe::Service {
    public:
        Service(const char* serviceName, DecodeAndExecute cb) : AndroidPipe::Service(serviceName), m_name(serviceName), m_cb(cb) {}

        virtual AndroidPipe* create(void* mHwPipe, const char* args) override {
            return new AndroidMessagePipe(mHwPipe, this, m_cb);
        }
        const char* name() {return m_name.c_str();}
    private:
        std::string m_name;
        DecodeAndExecute m_cb;
    };

    AndroidMessagePipe(void* hwPipe, Service* service, DecodeAndExecute cb);

    void onGuestClose(PipeCloseReason reason) override {}
    unsigned onGuestPoll() const override;
    int onGuestRecv(AndroidPipeBuffer* buffers, int numBuffers) override;
    int onGuestSend(const AndroidPipeBuffer* buffers,
                            int numBuffers) override;
    void onGuestWantWakeOn(int flags) override {}
    

private:

    void switchState();

    size_t copyData(uint8_t* & bufdata, size_t& bufsize, bool fromBufdata);

    int getSendPayloadSize();

    bool readyForGuestToReceive();

    bool readyForGuestToSend();

    enum class State {
        WaitingForGuestSendMesgLength,
        WaitingForGuestSendMesgPayload,
        WaitingForGuestRecvMesgLenth,
        WaitingForGuestRecvMesgPayload,
    };
    State mState = State::WaitingForGuestSendMesgLength;

    std::vector<uint8_t> mSendLengthBuffer;
    std::vector<uint8_t> mSendPayloadBuffer;
    std::vector<uint8_t> mRecvLengthBuffer;
    std::vector<uint8_t> mRecvPayloadBuffer;

    uint64_t m_expected = 0;
    uint8_t* m_curPos;
    DecodeAndExecute m_cb;

};

void registerAndroidMessagePipeService(const char* serviceName, android::AndroidMessagePipe::DecodeAndExecute cb);

}  // namespace android

