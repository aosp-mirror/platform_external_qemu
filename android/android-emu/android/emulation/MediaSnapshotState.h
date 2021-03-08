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

#pragma once
#include "android/base/files/Stream.h"

#include <stddef.h>
#include <list>
#include <vector>

namespace android {
namespace emulation {

class MediaSnapshotHelper;

class MediaSnapshotState {
public:
    struct ColorAspects {
        unsigned int primaries;
        unsigned int range;
        unsigned int transfer;
        unsigned int space;
    };

    struct PacketInfo {
        std::vector<uint8_t> data;
        uint64_t pts;
    };

    struct FrameInfo {
        std::vector<uint8_t> data;
        std::vector<uint32_t> texture;
        int width;
        int height;
        uint64_t pts;
        ColorAspects color;
    };

    bool savePacket(std::vector<uint8_t> data, uint64_t pts = 0);
    void saveSps(std::vector<uint8_t> xsps) { sps = std::move(xsps); }
    void savePps(std::vector<uint8_t> xpps) { pps = std::move(xpps); }

    void saveDecodedFrame(std::vector<uint8_t> data,
                          int width = 0,
                          int height = 0,
                          uint64_t pts = 0,
                          ColorAspects xcolor = ColorAspects{});

    void saveDecodedFrame(std::vector<uint32_t> texture,
                          int width = 0,
                          int height = 0,
                          uint64_t pts = 0,
                          ColorAspects xcolor = ColorAspects{});

    void saveDecodedFrame(FrameInfo frame) {
        savedFrames.push_back(std::move(frame));
    }

    void save(base::Stream* stream) const;
    void load(base::Stream* stream);

    FrameInfo* frontFrame() {
        if (savedFrames.empty()) {
            return nullptr;
        }
        return &(savedFrames.front());
    }

    void discardFrontFrame() {
        if (!savedFrames.empty()) {
            savedFrames.pop_front();
        }
    }

    friend MediaSnapshotHelper;

private:
    std::vector<uint8_t> sps;  // sps NALU
    std::vector<uint8_t> pps;  // pps NALU
    std::vector<PacketInfo> savedPackets;
    FrameInfo savedDecodedFrame;  // only one or nothing
    std::list<FrameInfo> savedFrames;

private:
    bool savePacket(const uint8_t* frame, size_t size, uint64_t pts = 0);

    //    void saveVec(base::Stream* stream, const std::vector<uint8_t>& vec)
    //    const;

    void saveFrameInfo(base::Stream* stream, const FrameInfo& frame) const;

    void savePacketInfo(base::Stream* stream, const PacketInfo& pkt) const;
    void saveColor(base::Stream* stream, const ColorAspects& color) const;

    void loadFrameInfo(base::Stream* stream, FrameInfo& frame);

    void loadPacketInfo(base::Stream* stream, PacketInfo& pkt);

    void loadColor(base::Stream* stream, ColorAspects& color) const;

    //    void loadVec(base::Stream* stream, std::vector<uint8_t>& vec);
};

}  // namespace emulation
}  // namespace android
