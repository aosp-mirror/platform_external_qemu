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

#include <pthread.h>

namespace android {
namespace display {

// defined in ExternalDisplayPipe.cpp
int external_display_reply_packet(Mirroring* mirroring,
                                  uint8_t* buffer,
                                  size_t size);

int Mirroring::sendSinkStatus(bool attached) {
    SinkAvailableMessage msg = {0};
    int size_to_send = attached ? sizeof(msg) : HEADER_SIZE;

    msg.header.id = SOURCE_ID;
    msg.header.what = attached ? MSG_SINK_AVAILABLE : MSG_SINK_NOT_AVAILABLE;
    msg.header.size = sizeof(msg) - HEADER_SIZE;

    msg.width = this->resolution_width;
    msg.height = this->resolution_height;
    msg.dpi = this->dpi;

    // use a lock so can be called from different threads
    VERBOSE_PRINT(extdisplay, "sendSinkStatus() starting...\n");
    android::base::AutoLock lock(send_lock);
    int res = external_display_reply_packet(this, (uint8_t*)&msg, size_to_send);
    VERBOSE_PRINT(extdisplay, "sendSinkStatus(), returned res=%d\n", res);

    this->state = (this->video_decoder) ? MIRRORING_DISPLAY_ATTACHED
                                        : MIRRORING_DISPLAY_DETACHED;

    return 0;
}

// h264 buffer is in annex-B format, with 00 00 00 01 start code
int Mirroring::processH264Frames(uint8_t* h264_buf, uint32_t h264_buf_size) {
    if (this->video_decoder != nullptr)
        this->video_decoder->decode(this->window, (const char*)h264_buf,
                                    h264_buf_size);

    return 0;
}

int Mirroring::processOneMessage(uint8_t* packet, int packet_len) {
    MessageHeader* mhdr = (MessageHeader*)packet;

    uint16_t message_type = mhdr->what;

    VERBOSE_PRINT(extdisplay,
                  "%s(): Message type: %d, size: %d, state=%d, header=%p",
                  __FUNCTION__, message_type, mhdr->size, this->state, mhdr);

    if (mhdr->id != SINK_ID) {
        derror("Invalid message due to id.\n");
        return -1;
    }

    if (mhdr->what == MSG_ACTIVATE_DISPLAY) {
        this->state = MIRRORING_DEVICE_CONNECTED;

        if (this->window == nullptr) {
            this->window = DisplayWindow::create(width, height);
            this->window->setMirroring(this);
        } else {
            this->window->resize(width, height);
        }
        this->video_decoder = FFmpegDecoder::create(
                0, nullptr, 0, this->resolution_width, this->resolution_height);
        if (this->video_decoder == nullptr) {
            derror("FFmpegDecoder: failed to create.\n");
        }
    
        return sendSinkStatus(true);
    }

    if (mhdr->what == MSG_CONTENT) {
        ContentMessage* content = (ContentMessage*)mhdr;
        return processH264Frames(content->payload, mhdr->size);
    }

    if (mhdr->what == MSG_DEACTIVATE_DISPLAY) {
        this->state = MIRRORING_DEVICE_DISCONNECTED;
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
                  __FUNCTION__, packet_len, this->packetbuf_pos);

    if (this->state == MIRRORING_DISPLAY_DETACHED ||
        this->state == MIRRORING_DEVICE_DISCONNECTED) {
        VERBOSE_PRINT(extdisplay, "%s(): ignore message", __FUNCTION__);
        return 0;
    }

    // allocate a buf to store a whole packet, one packet may come with a few
    // reads, and one read may contain a few packets
    if (this->packetbuf == nullptr) {
        // GNU libc malloc() always returns 8-byte aligned memory addresses
        this->packetbuf_size = 1024 * 1024 * 4;  // 4MB should be enough

#ifdef _WIN32
        this->packetbuf =
                (unsigned char*)_aligned_malloc(this->packetbuf_size, 32);
#elif __APPLE__
        // apple is always 16-byte aligned
        this->packetbuf = (unsigned char*)malloc(this->packetbuf_size);
#else
        this->packetbuf = (unsigned char*)memalign(32, this->packetbuf_size);
#endif

        this->packetbuf_pos = 0;
    }

    if (this->packetbuf == nullptr) {
        derror("%s(): aligned malloc failed.\n", __FUNCTION__);
        return -1;
    }

    if (this->packetbuf_pos + packet_len < this->packetbuf_size) {
        memcpy(this->packetbuf + this->packetbuf_pos, buffer, packet_len);
        packet_len = this->packetbuf_pos + packet_len;
        this->packetbuf_pos = 0;
    } else {
        derror("%s(): Packet size is too small. packet_len=%d, "
               "packetbuf_pos=%d, packetbuf_size=%d\n",
               __FUNCTION__, packet_len, this->packetbuf_pos,
               this->packetbuf_size);
        return -1;  // this should never happen
    }

    size_t pos = 0;
    size_t remaining = packet_len;
    size_t size;  // header size + payload size

    while (pos < packet_len && remaining > 0) {
        MessageHeader* header = (MessageHeader*)(this->packetbuf + pos);
        VERBOSE_PRINT(extdisplay, "%s(): header id=%d, what=%d, size=%d\n",
                      __FUNCTION__, header->id, header->what, header->size);

        // incomplete packet */
        size = HEADER_SIZE + header->size;
        if (pos + size > packet_len) {
            // message is not complete yet, issue an extra read
            VERBOSE_PRINT(extdisplay,
                          "%s(): Packet is incomplete, expecting %d bytes, but "
                          "we only got %d bytes, pos=%d.\n",
                          __FUNCTION__, size, remaining, pos);
            break;
        }

        int rc = processOneMessage(this->packetbuf + pos, size);
        if (rc < 0) {
            remaining = 0; // must be an invalid message
            break;
        }

        remaining -= size;
        pos += size;
    }

    this->packetbuf_pos = remaining;
    if (remaining > 0) {
        memmove(this->packetbuf, this->packetbuf + pos, remaining);
    }

    return 0;
}

int Mirroring::stop() {
    VERBOSE_PRINT(extdisplay, "%s() called", __FUNCTION__);

    if (this->video_decoder != nullptr) {
        delete this->video_decoder;
        this->video_decoder = nullptr;
    }

    if (this->window != nullptr) {
        this->window->remove();
        delete this->window;  
        this->window = nullptr;
    }

    this->packetbuf_pos = 0;

    return 0;
}

Mirroring::~Mirroring() {
    stop();

    if (this->packetbuf != nullptr) {
#ifdef _WIN32
        _aligned_free(this->packetbuf);
#else
        free(this->packetbuf);
#endif
    }

    this->packetbuf = nullptr;
}

}  // namespace display
}  // namespace android
