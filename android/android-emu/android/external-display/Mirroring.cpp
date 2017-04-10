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
//
// mirroring.cpp
//
// handles video stream from anodroid side, and render it on the
// host. By defualt, it mirrors the android screen. Second screen
// api aware apps can display different content
//

#include "android/external-display/Mirroring.h"

#include "android/external-display/DisplayWindow.h"
#include "android/external-display/FFmpegDecoder.h"
#include "android/external-display/protocol.h"

#include "android/utils/debug.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <ctype.h>

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#endif

namespace android {
namespace display {

// defined in ExternalDisplayPipe.cpp
int external_display_reply_packet(Mirroring* mirroring,
                                  uint8_t* buffer,
                                  size_t size);


// defined in SDLDisplayWindow.cpp
void registerSDLExternalDisplayWindowProvider();

ExternalDisplayWindowProvider* Mirroring::sProvider = nullptr;

int Mirroring::sendSinkStatus(bool attached) {
    SinkAvailableMessage msg = {0};
    int size_to_send = attached ? sizeof(msg) : HEADER_SIZE;

    msg.header.id = SOURCE_ID;
    msg.header.what = attached ? MSG_SINK_AVAILABLE : MSG_SINK_NOT_AVAILABLE;
    msg.header.size = sizeof(msg) - HEADER_SIZE;

    msg.width = this->mResolutionWidth;
    msg.height = this->mResolutionHeight;
    msg.dpi = this->mDpi;

    // use a lock so can be called from different threads
    VERBOSE_PRINT(extdisplay, "sendSinkStatus() starting...\n");
    android::base::AutoLock lock(mSendLock);
    int res = external_display_reply_packet(this, (uint8_t*)&msg, size_to_send);
    VERBOSE_PRINT(extdisplay, "sendSinkStatus(), returned res=%d\n", res);

    this->mState = (this->mVideoDecoder) ? MirroringDevState::DisplayAttached
                                         : MirroringDevState::DisplayDettached;    

    return 0;
}

// h264 buffer is in annex-B format, with 00 00 00 01 start code
int Mirroring::processH264Frames(uint8_t* h264_buf, uint32_t h264_buf_size) {
    if (this->mVideoDecoder != nullptr)
        this->mVideoDecoder->decode(this->mWindow, (const char*)h264_buf,
                                    h264_buf_size);

    return 0;
}

int Mirroring::processOneMessage(uint8_t* packet, int packet_len) {
    MessageHeader* mhdr = (MessageHeader*)packet;

    uint16_t message_type = mhdr->what;

    VERBOSE_PRINT(extdisplay,
                  "%s(): Message type: %d, size: %d, state=%d, header=%p",
                  __FUNCTION__, message_type, mhdr->size, this->mState, mhdr);

    if (mhdr->id != SINK_ID) {
        derror("Invalid message due to id.\n");
        return -1;
    }

    if (mhdr->what == MSG_ACTIVATE_DISPLAY) {
        this->mState = MirroringDevState::DeviceConnected;

        if (this->mWindow == nullptr) {
            //this->mWindow = DisplayWindow::create(mWidth, mHeight);
            if (sProvider != nullptr) {
                this->mWindow = sProvider->createWindow(mWidth, mHeight);
            }
            this->mWindow->setMirroring(this);
        } else {
            this->mWindow->resize(mWidth, mHeight);
        }
        this->mVideoDecoder = FFmpegDecoder::create(
                0, nullptr, 0, this->mResolutionWidth, this->mResolutionHeight);
        if (this->mVideoDecoder == nullptr) {
            derror("FFmpegDecoder: failed to create.\n");
        }
        return sendSinkStatus(true);
    }

    if (mhdr->what == MSG_CONTENT) {
        ContentMessage* content = (ContentMessage*)mhdr;
        return processH264Frames(content->payload, mhdr->size);
    }

    if (mhdr->what == MSG_DEACTIVATE_DISPLAY) {
        this->mState = MirroringDevState::DeviceDisconnected;
        stop();
        return sendSinkStatus(false);
    }

    return 0;
}

#ifndef MIN
#define MIN(a, b) ((a < b) ? a : b)
#endif

int Mirroring::processMessage(uint8_t* buffer, size_t packet_len) {
    VERBOSE_PRINT(extdisplay,
                  "%s(): Read (%d bytes) from bulk endpoint, packetbuf_pos=%d",
                  __FUNCTION__, packet_len, this->mPacketBufPos);

    if (this->mState == MirroringDevState::DeviceDisconnected ||
        this->mState == MirroringDevState::DisplayDettached) {
        VERBOSE_PRINT(extdisplay, "%s(): ignore message", __FUNCTION__);
        return 0;
    }

    // allocate a buf to store a whole packet, one packet may come with a few
    // reads, and one read may contain a few packets
    if (this->mPacketBuf == nullptr) {
        // GNU libc malloc() always returns 8-byte aligned memory addresses
        this->mPacketBufSize = 1024 * 1024 * 4;  // 4MB should be enough

#ifdef _WIN32
        this->mPacketBuf =
                (unsigned char*)_aligned_malloc(this->mPacketBufSize, 32);
#elif __APPLE__
        // apple is always 16-byte aligned
        this->mPacketBuf = (unsigned char*)malloc(this->mPacketBufSize);
#else
        this->mPacketBuf = (unsigned char*)memalign(32, this->mPacketBufSize);
#endif

        this->mPacketBufPos = 0;
    }

    if (this->mPacketBuf == nullptr) {
        derror("%s(): aligned malloc failed.\n", __FUNCTION__);
        return -1;
    }

    if (this->mPacketBufPos + packet_len < this->mPacketBufSize) {
        memcpy(this->mPacketBuf + this->mPacketBufPos, buffer, packet_len);
        packet_len = this->mPacketBufPos + packet_len;
        this->mPacketBufPos = 0;
    } else {
        derror("%s(): Packet size is too small. packet_len=%d, "
               "packetbuf_pos=%d, packetbuf_size=%d\n",
               __FUNCTION__, packet_len, this->mPacketBufPos,
               this->mPacketBufSize);
        return -1;  // this should never happen
    }

    size_t pos = 0;
    size_t remaining = packet_len;
    size_t size;  // header size + payload size

    while (pos < packet_len && remaining > 0) {
        MessageHeader* header = (MessageHeader*)(this->mPacketBuf + pos);
        VERBOSE_PRINT(extdisplay, "%s(): header id=%d, what=%d, size=%d\n",
                      __FUNCTION__, header->id, header->what, header->size);

        // incomplete packet
        size = HEADER_SIZE + header->size;
        if (pos + size > packet_len) {
            // message is not complete yet, issue an extra read
            VERBOSE_PRINT(extdisplay,
                          "%s(): Packet is incomplete, expecting %d bytes, but "
                          "we only got %d bytes, pos=%d.\n",
                          __FUNCTION__, size, remaining, pos);
            break;
        }

        int rc = processOneMessage(this->mPacketBuf + pos, size);
        if (rc < 0) {
            remaining = 0;  // must be an invalid message
            break;
        }

        remaining -= size;
        pos += size;
    }

    this->mPacketBufPos = remaining;
    if (remaining > 0) {
        memmove(this->mPacketBuf, this->mPacketBuf + pos, remaining);
    }

    return 0;
}

int Mirroring::stop() {
    VERBOSE_PRINT(extdisplay, "%s() called", __FUNCTION__);

    if (this->mVideoDecoder != nullptr) {
        delete this->mVideoDecoder;
        this->mVideoDecoder = nullptr;
    }

    if (this->mWindow != nullptr) {
        this->mWindow->remove();
        delete this->mWindow;
        this->mWindow = nullptr;
    }

    this->mPacketBufPos = 0;

    return 0;
}

Mirroring::Mirroring() {
    //@TODO move this call to emulator main
    registerSDLExternalDisplayWindowProvider();
}

Mirroring::~Mirroring() {
    stop();

    if (this->mPacketBuf != nullptr) {
#ifdef _WIN32
        _aligned_free(this->mPacketBuf);
#else
        free(this->mPacketBuf);
#endif
    }

    this->mPacketBuf = nullptr;
}

}  // namespace display
}  // namespace android
