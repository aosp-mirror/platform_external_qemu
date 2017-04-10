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
//
// handles video stream from anodroid side, and render it on the
// host. By defualt, it mirrors the android screen. Second screen
// api aware apps can display different content

#pragma once

#include "android/base/synchronization/Lock.h"

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"

namespace android {
namespace display {

enum class MirroringDevState {
    Unknown,
    DeviceConnected,
    DeviceDisconnected,
    DisplayAttached,
    DisplayDettached
};

class FFmpegDecoder;
class DisplayWindow;

class Mirroring {
public:
    Mirroring() = default;
    ~Mirroring();

    void setPlatformPrivate(void* priv) { this->mPlatformPrivate = priv; }
    void* getPlatformPrivate() { return this->mPlatformPrivate; }
    int processMessage(uint8_t* buffer, size_t size);
    int sendSinkStatus(bool attached);
    int stop();

private:
    int processH264Frames(uint8_t* h264_buf, uint32_t h264_buf_size);
    int processOneMessage(uint8_t* packet, int packet_len);

private:
    void* mPlatformPrivate = nullptr;

    // display resolution sent to device
    uint32_t mResolutionWidth = 1280;
    uint32_t mResolutionHeight = 720;
    uint32_t mDpi = 240;

    MirroringDevState mState = MirroringDevState::Unknown;

    android::base::Lock mSendLock;

    // packet buffer
    uint8_t* mPacketBuf = nullptr;
    uint32_t mPacketBufSize = 0;
    uint32_t mPacketBufPos = 0;

    // video decoding stuff
    DisplayWindow* mWindow = nullptr;
    FFmpegDecoder* mVideoDecoder = nullptr;
    uint32_t mWidth = 1280;
    uint32_t mHeight = 720;
};

}  // namespace display
}  // namespace android
