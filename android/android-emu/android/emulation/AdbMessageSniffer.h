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

#pragma once

#include "android/base/Log.h"
#include "android/emulation/AndroidPipe.h"
#include "android/globals.h"

#include <iostream>
#include <map>
#include <set>
#include <vector>

namespace android {
namespace emulation {

// An AdbMessageSniffer can intercept the adb message stream and output
// them to a std::ostream when a complete packet is received.
// Will only consume resources if the logging level > 1
class AdbMessageSniffer {
public:
    struct amessage {
        unsigned command;     /* command identifier constant      */
        unsigned arg0;        /* first argument                   */
        unsigned arg1;        /* second argument                  */
        unsigned data_length; /* length of payload (0 is allowed) */
        unsigned data_check;  /* checksum of data payload         */
        unsigned magic;       /* command ^ 0xffffffff             */
    };

    struct apacket {
        amessage mesg;
        uint8_t data[1024];
    };

    // Creates a new sniffer.
    // |name| The prefix printed for each log message.
    // |level| The requested logging level, only level > 1 will use resources.
    // |oStream| Stream to which to log the messages.
    AdbMessageSniffer(const char* name,
                      int level = android_hw->test_monitorAdb,
                      std::ostream* oStream = &std::cout);
    ~AdbMessageSniffer();

    void read(const AndroidPipeBuffer*, int numBuffers, int count);

protected:
    // Only used by unit test, needs to be called before any read message is
    // called.
    void setLevel(int level) { mLevel = level; };

private:
    apacket mPacket;
    int mState;
    uint8_t* mCurrPos;
    std::vector<uint8_t> mBufferVec;
    uint8_t* mBuffer;
    uint8_t* mBufferP;
    const std::string mName;
    int mLevel;
    std::ostream* mLogStream;

    std::set<unsigned> mDummyShellArg0;

    bool packetSeemsValid();
    void grow_buffer_if_needed(int count);
    int getAllowedBytesToPrint(int bytes);
    bool checkForDummyShellCommand();
    size_t getPayloadSize();
    void copyFromBuffer(size_t count);
    int readPayload(int dataSize);
    void printMessage();
    void updateCtsHeartBeatCount();
    void printPayload();
    const char* getCommandName(unsigned code);
    void startNewMessage();
    int readHeader(int inputDataSize);
    void readToBuffer(const AndroidPipeBuffer* buffers,
                      int numBuffers,
                      int inputDataSize);
    void parseBuffer(int bufferLength);
};

}  // namespace emulation
}  // namespace android
