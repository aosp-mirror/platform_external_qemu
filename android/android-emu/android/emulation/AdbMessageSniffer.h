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
#include <cstdint>
#include "aemu/base/logging/Log.h"

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
        uint32_t command;     /* command identifier constant      */
        uint32_t arg0;        /* first argument                   */
        uint32_t arg1;        /* second argument                  */
        uint32_t data_length; /* length of payload (0 is allowed) */
        uint32_t data_check;  /* checksum of data payload         */
        uint32_t magic;       /* command ^ 0xffffffff             */
    };

    virtual ~AdbMessageSniffer() = default;
    virtual void read(const AndroidPipeBuffer*, int numBuffers, int count) = 0;

    // Creates a new sniffer.
    // |name| The prefix printed for each log message.
    // |level| The requested logging level, only level > 0 will use resources.
    // |oStream| Stream to which to log the messages.
    static AdbMessageSniffer* create(const char* name,
                                     int level);

};

}  // namespace emulation
}  // namespace android
