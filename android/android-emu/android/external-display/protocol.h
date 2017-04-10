// Copyright 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details

// protocol.h
// 
// This should be consistent with android side code in 
// device/generic/goldfish/RemoteDisplayProvider/src/com/android/emulator
// /remotedisplay/Protocol.java
//

#pragma once

namespace android {
namespace display {

#ifdef _MSC_VER
#define __attribute__(a)
#endif

#define HEADER_SIZE	8

#ifdef _MSC_VER
#pragma pack(push, 4)
#endif

// Message header.
//   0: service id (16 bits)
//   2: what (16 bits)
//   4: content size (32 bits)
//   8: ... content follows ...
struct __attribute__((__packed__)) MessageHeader
{
    uint16_t id;
    uint16_t what;
    uint32_t size;
};
#ifdef _MSC_VER
#pragma pack(pop)
#endif

#ifdef _MSC_VER
#pragma pack(push, 4)
#endif

// Message header.
//   0: service id (16 bits)
//   2: what (16 bits)
//   4: content size (32 bits)
//   8: ... content follows ...
struct __attribute__((__packed__)) ContentMessage
{
    MessageHeader header;
    uint8_t payload[0];
};
#ifdef _MSC_VER
#pragma pack(pop)
#endif

// send from source to sink

#define SOURCE_ID 2

// Tell sink to add a display with given width and height
// Replies with sink available or not available.
#define MSG_ACTIVATE_DISPLAY 1

// Send MPEG2-TS H.264 encoded content.
#define MSG_CONTENT 2

// Tell sink to remove a display
// Reply with MSG_SINK_NOT_AVAILABLE
#define MSG_DEACTIVATE_DISPLAY 3

#define MSG_QUERY_EXT 100

// send from sink to source

#define SINK_ID 1

// Sink is now available for use.
//   0: width (32 bits)
//   4: height (32 bits)
//   8: density dpi (32 bits)
#define MSG_SINK_AVAILABLE 1

#ifdef _MSC_VER
#pragma pack(push, 4)
#endif

struct __attribute__((__packed__)) SinkAvailableMessage
{
    MessageHeader header;
    uint32_t width;
    uint32_t height;
    uint32_t dpi;
};
#ifdef _MSC_VER
#pragma pack(pop)
#endif

// Sink is no longer available for use.
#define MSG_SINK_NOT_AVAILABLE 2

#define MSG_SINK_INJECT_EVENT 100

} // namespace display
} // namespace android

