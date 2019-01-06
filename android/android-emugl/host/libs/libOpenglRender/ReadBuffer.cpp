/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "ReadBuffer.h"

#include "android/base/system/System.h"

#include "ErrorLog.h"

#include <algorithm>

#include <assert.h>
#include <string.h>
#include <limits.h>

using android::base::System;

namespace emugl {

ReadBuffer::ReadBuffer(size_t bufsize) {
    m_size = bufsize;
    m_buf = (unsigned char*)malloc(m_size);
    m_validData = 0;
    m_readPtr = m_buf;
}

ReadBuffer::~ReadBuffer() {
    free(m_buf);
}

int ReadBuffer::getDataSharedMem(ChannelStream* stream, int minSize) {
    // fprintf(stderr, "%s:%p valid data: %zu want total: %d\n", __func__, stream, m_validData, minSize);
    // if (m_validData != 0) {
    //  fprintf(stderr, "%s: sharedMem read with previous valid data!\n", __func__);
    //  abort();
    // }

    // assume we are in hung up mode
    uint32_t fromGuestTotal = 0;

    uint32_t minSizeToRead = minSize - m_validData;

    uint32_t readAlready = (uint32_t)(uintptr_t)(m_readPtr - m_buf);
    uint32_t currentOccupancy = readAlready + m_validData;

    while (fromGuestTotal < minSizeToRead) {
        uint32_t fromGuestSize = stream->waitForGuestPingAndTrafficSize();

        if (fromGuestSize == 0) {
            // fprintf(stderr, "%s: need exit\n", __func__);
            if (fromGuestTotal > 0) {
                return fromGuestTotal;
            } else {
                return -1;
            }
        }

        fromGuestTotal += fromGuestSize;
        currentOccupancy += fromGuestSize;

        // fprintf(stderr, "%s:%p got %u (total: %u)\n", __func__, stream, fromGuestSize, fromGuestTotal);

        if (currentOccupancy > m_size ||
            minSize > m_size) {
            
            // fprintf(stderr, "%s: service new alloc. currOcc %u readAlready %u validData %u minSize %u limit %u (2x: %u)\n", __func__, currentOccupancy, readAlready, m_validData, minSize, m_size, m_size * 2);

            size_t needed =
                minSize > currentOccupancy ? minSize : currentOccupancy;

            if (readAlready >= needed &&
                readAlready >= m_validData) {
                // fprintf(stderr, "%s: with memcpy\n", __func__);
                memcpy(m_buf, m_readPtr, m_validData);
                m_readPtr = m_buf;
            } else {
                size_t bigger =
                    (minSize > currentOccupancy) ?
                    minSize : currentOccupancy;
                
                m_size = bigger * 2;

                unsigned char* new_buf = (unsigned char*)malloc(m_size);
                memcpy(new_buf, m_readPtr, m_validData);
                free(m_buf);
                m_buf = new_buf;
                m_readPtr = m_buf;
            }
        }

        stream->readFullyAndHangUp(m_readPtr + m_validData, fromGuestSize);

        m_validData += fromGuestSize;
    }

    // fprintf(stderr, "%s:%p done with %u\n", __func__, stream, fromGuestTotal);
    return (int)fromGuestTotal;
}

int ReadBuffer::getData(IOStream* stream, int minSize) {
    assert(stream);
    assert(minSize > (int)m_validData);

uint64_t start_us = System::get()->getHighResTimeUs();

    const int minSizeToRead = minSize - m_validData;
    int maxSizeToRead;
    const int freeTailSize = m_buf + m_size - (m_readPtr + m_validData);
    if (freeTailSize >= minSizeToRead) {
        maxSizeToRead = freeTailSize;
    } else {
        if (freeTailSize + (m_readPtr - m_buf) >= minSizeToRead) {
            // There's some gap in the beginning, if we move the data over it
            // that's going to be enough.
            memmove(m_buf, m_readPtr, m_validData);
        } else {
            // Not enough space even with moving, reallocate.
            // Note: make sure we can fit at least two of the requested packets
            //  into the new buffer to minimize the reallocations and
            //  memmove()-ing stuff around.
            size_t new_size = std::max<size_t>(
                    2 * minSizeToRead + m_validData,
                    2 * m_size);
            if (new_size < m_size) {  // overflow check
                new_size = INT_MAX;
            }

            const auto new_buf = (unsigned char*)malloc(new_size);
            if (!new_buf) {
                ERR("Failed to alloc %zu bytes for ReadBuffer\n", new_size);
                return -1;
            }

            memcpy(new_buf, m_readPtr, m_validData);
            free(m_buf);
            m_buf = new_buf;
            m_size = new_size;
        }
        // We can read more now, let's request it in case all data is ready
        // for reading.
        maxSizeToRead = m_size - m_validData;
        m_readPtr = m_buf;
    }

uint64_t prep_end_us = System::get()->getHighResTimeUs();

m_timePrepping += (prep_end_us - start_us);

    // get fresh data into the buffer;
    int readTotal = 0;
    do {
        const size_t readNow = stream->read(m_readPtr + m_validData,
                                            maxSizeToRead - readTotal);
        if (!readNow) {
            if (readTotal > 0) {

                uint64_t read_end_us0 = System::get()->getHighResTimeUs();
                m_timeChannelReading += (read_end_us0 - prep_end_us);

                return readTotal;
            } else {
                uint64_t read_end_us1 = System::get()->getHighResTimeUs();
                m_timeChannelReading += (read_end_us1 - prep_end_us);
                return -1;
            }
        }
        readTotal += readNow;
        m_validData += readNow;
    } while (readTotal < minSizeToRead);

                uint64_t read_end_us2 = System::get()->getHighResTimeUs();
                m_timeChannelReading += (read_end_us2 - prep_end_us);
    return readTotal;
}

void ReadBuffer::printAndResetStats() {
    fprintf(stderr, "%s: time prepping: %f s channel reading: %f s\n", __func__,
            m_timePrepping / 1000000.0f,
            m_timeChannelReading / 1000000.0f);
    m_timePrepping = 0;
    m_timeChannelReading = 0;
}

void ReadBuffer::consume(size_t amount) {
    assert(amount <= m_validData);
    m_validData -= amount;
    m_readPtr += amount;

    if (m_validData == 0) {
        m_readPtr = m_buf;
    }
}

void ReadBuffer::onSave(android::base::Stream* stream) {
    stream->putBe32(m_size);
    stream->putBe32(m_validData);
    stream->write(m_readPtr, m_validData);
}

void ReadBuffer::onLoad(android::base::Stream* stream) {
    const auto size = stream->getBe32();
    if (size > m_size) {
        m_size = size;
        free(m_buf);
        m_buf = (unsigned char*)malloc(m_size);
    }
    m_readPtr = m_buf;
    m_validData = stream->getBe32();
    assert(m_validData <= m_size);
    stream->read(m_readPtr, m_validData);
}

}  // namespace emugl
