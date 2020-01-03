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
#include <ostream>

class AndroidPipeBuffer;
namespace android {
namespace emulation {

// An AdbMessageSniffer can intercept the adb message stream and output
// them to a std::ostream when a complete packet is received.
// packets will be printed while the stream contains valid packets.
// Should only consume resources if the logging level > 0
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

    virtual ~AdbMessageSniffer() = default;
    virtual void read(const AndroidPipeBuffer*, int numBuffers, int count) = 0;

    // Creates a new sniffer.
    // |name| The prefix printed for each log message.
    // |level| The requested logging level, only level > 0 will use resources.
    // |oStream| Stream to which to log the messages.
    static AdbMessageSniffer* create(const char* name,
                                     int level,
                                     std::ostream* oStream);
};

}  // namespace emulation
}  // namespace android
