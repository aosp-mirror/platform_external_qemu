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

// GLProtocol adds checksum to GL pipe communication.
// It can:
//	(1) take a buffer and compute its checksum string;
//	(2) take a list of buffers one by one and compute their checksum string,
//	    in this case the checksum should be as the data in those buffers are
//	    concatenated;
//	(3) take a buffer and a checksum string, tell if they match

class GLProtocol {
public:
    GLProtocol();
    
    uint32_t getVersion() { return m_version; }
    bool setVersion(uint32_t version);

    static const uint32_t maxVersion = 1;
    static const char* getMaxVersionString();
    
    // Compute checksum for a packet and write it into checksumBuf
    // The size of checksumBuf should be queried from checksumStrSize())
    void checksumStr(const void *buf, size_t packetLen, void *checksumBuf);
    size_t checksumStrSize() const;

    // Compute checksum by taking buffers one by one
    // Call addBuffer to add all the buffers then call checksumFromBuffers
    // to get the checksum.
    // Internally each call of addBuffer might change the state of
    // m_partialChecksumData, while checksumFromBuffers will clear it.
    void addBuffer(const void *buf, size_t packetLen);
    // The 
    void checksumStrFromBuffers(void *outputChecksum);
    
    // Calculate the checksum of a packet (with size specified by packetLen),
    // and compare it with the checksum encoded in
    // buf + packetLen~packetLen+checksumStrSize
    bool validate(const void *buf, size_t packetLen);
protected:
    uint32_t m_version;
    // A temporary state used to compute the total length of a list of buffers,
    // if addBuffer is called.
    // Only used in protocol version 1.
    uint32_t m_v1bufferTotalLength;
    // Compute a 32bit checksum
    // Used in protocol v1
    uint32_t checksum(const void *buf, size_t packetLen);
};

#endif /* GLPROTOCOL_H */

