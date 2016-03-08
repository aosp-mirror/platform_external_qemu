/*
* Copyright (C) 2016 The Android Open Source Project
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

#include "ChecksumHelper.h"

#include <string>
#include <string.h>
#include <vector>

// change CHECKSUMHELPER_MAX_VERSION when you want to update the protocol version
#define CHECKSUMHELPER_MAX_VERSION 1

// utility macros to create checksum string at compilation time
#define CHECKSUMHELPER_VERSION_STR_PREFIX "ANDROID_EMU_CHECKSUM_HELPER_v"
#define CHECKSUMHELPER_MACRO_TO_STR(x) #x
#define CHECKSUMHELPER_MACRO_VAL_TO_STR(x) CHECKSUMHELPER_MACRO_TO_STR(x)

const uint32_t ChecksumHelper::kMaxVersion = CHECKSUMHELPER_MAX_VERSION;
const char* ChecksumHelper::kMaxVersionStrPrefix = CHECKSUMHELPER_VERSION_STR_PREFIX;
const char* ChecksumHelper::kMaxVersionStr = CHECKSUMHELPER_VERSION_STR_PREFIX CHECKSUMHELPER_MACRO_VAL_TO_STR(CHECKSUMHELPER_MAX_VERSION);

#undef CHECKSUMHELPER_MAX_VERSION
#undef CHECKSUMHELPER_VERSION_STR_PREFIX
#undef CHECKSUMHELPER_MACRO_TO_STR
#undef CHECKSUMHELPER_MACRO_VAL_TO_STR

ChecksumHelper::ChecksumHelper() :
    m_version(0),
    m_v1BufferTotalLength(0),
    m_numRead(0),
    m_numWrite(0),
    m_isEncodingChecksum(false)
{ }

bool ChecksumHelper::setVersion(uint32_t version) {
    if (version > kMaxVersion) {  // unsupported version
        LOG_CHECKSUMHELPER("%s: ChecksumHelper Set Unsupported version Version %d\n",
                __FUNCTION__, m_version);
        return false;
    }
    if (m_isEncodingChecksum) { // setVersion is called in the middle of encoding checksums
        LOG_CHECKSUMHELPER("%s: called between addBuffer and writeChecksum\n",
                __FUNCTION__);
        return false;
    }
    m_version = version;
    LOG_CHECKSUMHELPER("%s: ChecksumHelper Set Version %d\n", __FUNCTION__,
                m_version);
    return true;
}

size_t ChecksumHelper::checksumBitSize() const {
    switch (m_version) {
        case 0:
            return 0;
        case 1:
            return 8;
        default:
            return 0;
    }
}

void ChecksumHelper::addBuffer(const void* buf, size_t packetLen) {
    m_isEncodingChecksum = true;
    switch (m_version) {
        case 1:
            m_v1BufferTotalLength += packetLen;
            break;
    }
}

void ChecksumHelper::writeChecksum(void* outputChecksum) {
    char *checksumPtr = (char *)outputChecksum;
    switch (m_version) {
        case 1: { // protocol v1 is to reverse the packetLen and write it at the end
            uint32_t val = computeV1Checksum(NULL, m_v1BufferTotalLength);
            memcpy(checksumPtr, &val, 4);
            memcpy(checksumPtr+4, &m_numWrite, 4);
            break;
        }
    }
    resetChecksum();
    m_isEncodingChecksum = false;
    m_numWrite++;
}

void ChecksumHelper::resetChecksum() {
    switch (m_version) {
        case 1:
            m_v1BufferTotalLength = 0;
            break;
    }
    m_isEncodingChecksum = false;
}

bool ChecksumHelper::validate(const void* buf, size_t bufLen, const void* expectedChecksum) {
    // buffers for computing the checksum
    const size_t kMaxChecksumSize = 8;
    static unsigned char sChecksumBuffer[kMaxChecksumSize];

    size_t checksumSize = checksumBitSize();
    switch (m_version) {
        case 1: {
            uint32_t val = computeV1Checksum(buf, bufLen);
            memcpy(sChecksumBuffer, &val, 4);
            memcpy(sChecksumBuffer+4, &m_numRead, 4);
            break;
        }
    }
    bool isValid = !memcmp(sChecksumBuffer, expectedChecksum, checksumSize);
    m_numRead++;
    return isValid;
}

uint32_t ChecksumHelper::computeV1Checksum(const void* buf, size_t bufLen) {
    uint32_t revLen = bufLen;
    revLen = (revLen & 0xffff0000) >> 16 | (revLen & 0x0000ffff) << 16;
    revLen = (revLen & 0xff00ff00) >> 8 | (revLen & 0x00ff00ff) << 8;
    revLen = (revLen & 0xf0f0f0f0) >> 4 | (revLen & 0x0f0f0f0f) << 4;
    revLen = (revLen & 0xcccccccc) >> 2 | (revLen & 0x33333333) << 2;
    revLen = (revLen & 0xaaaaaaaa) >> 1 | (revLen & 0x55555555) << 1;
    return revLen;
}
