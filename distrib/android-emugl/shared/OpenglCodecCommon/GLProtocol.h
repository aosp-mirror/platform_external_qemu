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

#ifndef GLPROTOCOL_H
#define GLPROTOCOL_H

#include <stdint.h>
#include <stdlib.h>

// Set LOG_GLPROTOCOL to 1 to debug creation/destruction of GLprotocol
// instances.
#define TRACE_GLPROTOCOL 1

#if TRACE_GLPROTOCOL
#define LOG_GLPROTOCOL(x...) fprintf(stderr, x)
#else
#define LOG_GLPROTOCOL(x...)
#endif

class GLProtocol {
public:
    GLProtocol() : m_version(0) { }
    
    uint32_t getVersion() { return m_version; }
    bool setVersion(uint32_t version);

    static const uint32_t maxVersion = 1;
    static const char* getMaxVersionString();
    
    // Compute checksum for a packet and write it into checksumBuf
    // The size of checksumBuf should be queried from checksumStrSize())
    void checksumStr(const void *buf, size_t packetLen, void *checksumBuf);
    size_t checksumStrSize() const;

    // Calculate the checksum of a packet (with size specified by packetLen),
    // and compare it with the checksum encoded in
    // buf + packetLen~packetLen+checksumStrSize
    bool validate(const void *buf, size_t packetLen);
protected:
    // Decode the checksum from a buffer
    // In version 1 it simply loads the next 4 bytes in the buffer
    uint32_t m_version;
};

#endif /* GLPROTOCOL_H */

