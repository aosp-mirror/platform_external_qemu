// Copyright (C) 2019 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// A parser for H.264 bitstream. It will determine will kind of Netowrk
// Abstraction Layer Unit (NALU) we have received from the guest.

#pragma once

#include <cstdint>
#include <string>

namespace android {
namespace emulation {

// All H.264 frames are structured in the following way:
//
// ===============================================================================
// | start code header | forbidden zero bit | nal_ref_idc | nal_unit_type | data |
// | 3 or 4 bytes      | 1 bit              | 2 bits      | 5 bits        | any  |
// ===============================================================================
//
// - Start code header:
//
//   Can be either 0x00 00 01 or 0x00 00 00 01. The start code header indicates where
//   each Network Abstraction Layer Unit (NALU) begins, since this is a continuous
//   stream of data.
//
// - forbidden zero bit:
//
//   Used as an error-indicator. 0 means the transmission is normal, while 1 means there
//   is a syntax violation.
//
// - nal_ref_idc:
//
//   Indicates whether the NALU is a reference field, frame or picture.
//
// - nal_unit_type:
//
//   The type of data contained in the frame. See H264NaluType below for a list of the types.
//

class H264NaluParser {
public:
enum class H264NaluType : uint8_t {
    Unspecified0 = 0,
    CodedSliceNonIDR = 1,
    CodedSlicePartitionA = 2,
    CodedSlicePartitionB = 3,
    CodedSlicePartitionC = 4,
    CodedSliceIDR = 5,
    SEI = 6,
    SPS = 7,
    PPS = 8,
    AccessUnitDelimiter = 9,
    EndOfSequence = 10,
    EndOfStream = 11,
    FillerData = 12,
    SPSExtension = 13,
    PrefixNALU = 14,
    SPSSubset = 15,
    Reserved0 = 16,
    Reserved1 = 17,
    Reserved2 = 18,
    CodedSliceAuxiliary = 19,
    CodedSliceExtension = 20,
    CodedSliceExtDepthComponents = 21,
    Reserved3 = 22,
    Reserved4 = 23,
    STAP_A = 24,
    STAP_B = 25,
    MTAP16 = 26,
    MTAP24 = 27,
    FU_A = 28,
    FU_B = 29,
    Unspecified1 = 30,
    Unspecified2 = 31,
    Undefined = 32, // not in H.264 nalu type; just here to cap the enums so checking is simpler.
};

static const std::string kNaluTypesStrings[];

static bool checkSpsFrame(const uint8_t* frame, size_t szBytes);
static bool checkPpsFrame(const uint8_t* frame, size_t szBytes);
static bool checkIFrame(const uint8_t* frame, size_t szBytes);
// Returns the description of the H264NaluType.
static const std::string& naluTypeToString(H264NaluType n);
// Parses |frame| for the NALU type. |szBytes| is the size of |frame|, |data| will be set to
// the location after the start code header. This means that the first byte of |data| will still
// contain the NALU type in the lower 5-bits.
static H264NaluType getFrameNaluType(const uint8_t* frame, size_t szBytes, uint8_t** data = nullptr);

// Finds the position of the next start code header. Returns null if not found, otherwise returns
// a pointer to the beginning of the next start code header.
static const uint8_t* getNextStartCodeHeader(const uint8_t* frame, size_t szBytes);
};

}  // namespace emulation
}  // namespace android
