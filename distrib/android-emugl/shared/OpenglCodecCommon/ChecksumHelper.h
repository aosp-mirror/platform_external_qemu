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

#pragma once

#include <stdint.h>
#include <stdlib.h>

// Set TRACE_CHECKSUMHELPER to 1 to debug creation/destruction of GLprotocol
// instances.
#define TRACE_CHECKSUMHELPER 0

#if TRACE_CHECKSUMHELPER
#define LOG_CHECKSUMHELPER(x...) fprintf(stderr, x)
#else
#define LOG_CHECKSUMHELPER(x...)
#endif

// ChecksumHelper adds checksum as an array of bytes to GL pipe communication, which
// size depends on the protocol version. Each pipe should use one ChecksumHelper.
// It can:
//      (1) take a list of buffers one by one and compute their checksum string,
//          in this case the checksum should be as the data in those buffers are
//          concatenated;
//      (2) take a buffer and a checksum string, tell if they match;
//      (3) support different checksum version in future.
//
// For backward compatibility, checksum version 0 behaves the same as there is
// no checksum (i.e., checksumBitSize returns 0, validate always returns true,
// addBuffer and writeCheckSum does nothing).
//
// Notice that to detect package lost, ChecksumHelper also keeps track of how
// many times it generates/validates checksums, and might use it as part of the
// checksum.
//
// To evaluate checksums from a list of data buffers buf1, buf2... Please call
// addBuffer(buf1, buf1len), addBuffer(buf2, buf2len) ... in order.
// Then allocate a checksum buffer with size checksumBitSize(), and call
// writeChecksum(checksumBuffer) to write the checksum to the buffer.
//
// To calculate if a data buffer match its checksum, please call
// validate(buf, bufLen, checksumBuffer)
//
// The checksum generator and validator must be set to the same version, and
// the validator must check ALL checksums in the order they are generated,
// otherwise the validation function will return false.
//
// It is allowed to change the checksum version between calculating two
// checksums. This is designed for backward compatibility reason.
//
// Example:
//
// bool testChecksum(void* buf, size_t bufLen) {
//     // encoding message
//     ChecksumHelper encoder;
//     encoder.setVersion(1);
//     encoder.addBuffer(buf, bufLen);
//     std::vector<unsigned char> message(bufLen + checksumBitSize());
//     memcpy(&message[0], buf, bufLen);
//     encoder.writeChecksum(&message[0] + bufLen);
//
//     // decoding message
//     ChecksumHelper decoder;
//     decoder.setVersion(1);
//     return decoder.validate(&message[0], bufLen, &message[0] + bufLen);
// }

class ChecksumHelper {
public:
    ChecksumHelper();

    // Get and set current checksum version
    uint32_t getVersion() const { return m_version; }
    // setVersion should not be called between addBuffer and writeChecksum,
    // or it will return false.
    // Also setting version > getMaxVersion() returns false.
    bool setVersion(uint32_t version);

    // Maximum supported checksum version
    static uint32_t getMaxVersion();
    // A version string that looks like "ANDROID_EMU_CHECKSUM_HELPER_v1"
    // Used multiple times when the guest queries the maximum supported version
    // from the host.
    // The library owns the returned pointer. The returned pointer will be
    // deconstructed when unloading library.
    static const char* getMaxVersionStr();
    static const char* getMaxVersionStrPrefix();

    // Size of checksum in the current version
    size_t checksumBitSize() const;

    // Update the current checksum value from the data
    // at |buf| of |bufLen| bytes. Once all buffers
    // have been added, call writeChecksum() to store
    // the final checksum value and reset its state.
    void addBuffer(const void* buf, size_t bufLen);
    // Write the checksum from the list of buffers to outputChecksum
    void writeChecksum(void* outputChecksum);
    // Restore the states for computing checksums.
    // Automatically called at the end of writeChecksum.
    // Can also be used to abandon the current checksum being calculated.
    void resetChecksum();

    // Calculate the checksum of a packet (with size specified by packetLen),
    // and compare it with the checksum encoded in expectedChecksum
    bool validate(const void* buf, size_t bufLen, const void* expectedChecksum);
protected:
    uint32_t m_version;
    // A temporary state used to compute the total length of a list of buffers,
    // if addBuffer is called.
    uint32_t m_numRead;
    uint32_t m_numWrite;
    // m_isEncodingChecksum is true when between addBuffer and writeChecksum
    bool m_isEncodingChecksum;
private:
    // Compute a 32bit checksum
    // Used in protocol v1
    static uint32_t computeV1Checksum(const void* buf, size_t bufLen);
    // The buffer used in protocol version 1 to compute checksum.
    uint32_t m_v1BufferTotalLength;
};
