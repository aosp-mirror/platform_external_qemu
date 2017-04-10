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
//
// mirroring.h

#pragma once

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "protocol.h"

namespace android {
namespace display {

enum mirroring_dev_state {
    MIRRORING_UNKNOWN = 0,
    MIRRORING_DEVICE_CONNECTED = 1,
    MIRRORING_DEVICE_DISCONNECTED = 2,

    MIRRORING_DISPLAY_ATTACHED = 3,
    MIRRORING_DISPLAY_DETACHED = 4
};

class FFmpegDecoder;
class DisplayWindow;

class Mirroring {
public:
    Mirroring() = default;
    ~Mirroring();

    void setPlatformPrivate(void* priv) { this->platform_private = priv; }

    void* getPlatformPrivate() { return this->platform_private; }

    int processMessage(uint8_t* buffer, size_t size);
    int stop();

private:
    int sendSinkStatus(bool attached);
    int processH264Frames(uint8_t* h264_buf, uint32_t h264_buf_size);
    int processOneMessage(uint8_t* packet, int packet_len);

private:
    void* platform_private = nullptr;

    // display resolution sent to device
    uint32_t resolution_width = 1280;
    uint32_t resolution_height = 720;
    uint32_t dpi = 240;

    pthread_mutex_t send_mutex;

    int id;

    enum mirroring_dev_state state;

    // packet buffer
    uint8_t* packetbuf = nullptr;
    uint32_t packetbuf_size = 0;
    uint32_t packetbuf_pos = 0;

    // video decoding stuff
    DisplayWindow* window = nullptr;
    FFmpegDecoder* video_decoder = nullptr;
    uint32_t width = 1280;
    uint32_t height = 720;
};

}  // namespace display
}  // namespace android
