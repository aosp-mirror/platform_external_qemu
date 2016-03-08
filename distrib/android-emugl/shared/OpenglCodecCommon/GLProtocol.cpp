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

#include "GLProtocol.h"

#include <string.h>
#include <string>

bool GLProtocol::setVersion(uint32_t version) {
    if (version>maxVersion) return false; // unsupported version
    m_version = version;
    return true;
}

namespace {
    std::string maxVersionStr;
}

GLProtocol::GLProtocol() : m_version(0), m_v1bufferTotalLength(0) { }

const char* GLProtocol::getMaxVersionString() {
    if (maxVersionStr.length() == 0) {
        // A buffer to change 32bit integer to string.
        // Size is 11 because the maximum 32bit integer has 10 digits (and +1 for the ending).
        char buffer[11];
        sprintf(buffer, "%d", maxVersion);
        maxVersionStr = std::string("ANDROID_EMU_GL_PROTOCOL_") + buffer;
        LOG_GLPROTOCOL("%s: GLprotocol Version %s\n", __FUNCTION__,
                maxVersionStr.c_str());
    }
    return maxVersionStr.c_str();
}

void GLProtocol::checksumStr(const void *buf, size_t packetLen, void *checksumBuf) {
    char *checksumPtr = (char *)checksumBuf;
    switch (m_version) {
        case 1: {
            uint32_t val = checksum(buf, packetLen);
            memcpy(checksumPtr, &val, 4);
            memset(checksumPtr+4, 0, 4);
            break;
        }
    }
}

size_t GLProtocol::checksumStrSize() const {
    switch (m_version) {
        case 0:
            return 0;
        case 1:
            return 8;
        default:
            return 0;
    }
}

void GLProtocol::addBuffer(const void *buf, size_t packetLen) {
    switch (m_version) {
        case 1:
	    m_v1bufferTotalLength += packetLen;
	    break;
    }
}

void GLProtocol::checksumStrFromBuffers(void *outputChecksum) {
    char *checksumPtr = (char *)outputChecksum;
    switch (m_version) {
        case 1: { // protocol v1 is to reverse the packetLen and write it at the end
            uint32_t val = checksum(NULL, m_v1bufferTotalLength);
            memcpy(checksumPtr, &val, 4);
            memset(checksumPtr+4, 0, 4);
	    m_v1bufferTotalLength = 0;
            break;
        }
    }
}

bool GLProtocol::validate(const void *buf, size_t packetLen) {
    size_t checksumSize = checksumStrSize();
    unsigned char *checksumBuf = (unsigned char *)malloc(checksumSize);
    checksumStr(buf, packetLen, checksumBuf);
    const unsigned char *ptr = (unsigned char *)buf;
    bool isValid = !memcmp(checksumBuf, ptr + packetLen, checksumSize);
    free(checksumBuf);
    return isValid;
}

uint32_t GLProtocol::checksum(const void *buf, size_t packetLen) {
    uint32_t val = 0;
    switch (m_version) {
	case 1: // protocol v1 is to reverse the packetLen and write it at the end
            uint32_t revLen = packetLen;
            revLen = (revLen & 0xffff0000) >> 16 | (revLen & 0x0000ffff) << 16;
            revLen = (revLen & 0xff00ff00) >> 8 | (revLen & 0x00ff00ff) << 8;
            revLen = (revLen & 0xf0f0f0f0) >> 4 | (revLen & 0x0f0f0f0f) << 4;
            revLen = (revLen & 0xcccccccc) >> 2 | (revLen & 0x33333333) << 2;
            revLen = (revLen & 0xaaaaaaaa) >> 1 | (revLen & 0x55555555) << 1;
	    val = revLen;
	    break;
    }
    return val;
}