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
    switch (m_version) {
        case 0:
            break;
        case 1:
	    memset(checksumBuf, 0, 8);
            break;
        default:
            break;
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

bool GLProtocol::validate(const void *buf, size_t packetLen) {
    size_t checksumSize = checksumStrSize();
    unsigned char *checksumBuf = (unsigned char *)malloc(checksumSize);
    checksumStr(buf, packetLen, checksumBuf);
    const unsigned char *ptr = (unsigned char *)buf;
    bool isValid = !memcmp(checksumBuf, ptr + packetLen, checksumSize);
    free(checksumBuf);
    return isValid;
}
