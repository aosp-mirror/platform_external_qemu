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

#include "android/emulation/GoldfishMediaDefs.h"
#include "android/emulation/MediaH264Decoder.h"

#include <stddef.h>
#include <vector>

namespace android {
namespace emulation {

// this is an interface for platform specific implementations
// such as VideoToolBox, CUVID, etc
class MediaH264DecoderPlugin {
public:
    using PixelFormat = MediaH264Decoder::PixelFormat;
    using Err = MediaH264Decoder::Err;

    virtual void initH264Context(void* ptr) = 0;
    virtual void reset(void* ptr) = 0;
    virtual MediaH264DecoderPlugin* clone() = 0;
    virtual void destroyH264Context() = 0;
    virtual void decodeFrame(void* ptr) = 0;
    virtual void flush(void* ptr) = 0;
    virtual void getImage(void* ptr) = 0;

    virtual void save(base::Stream* stream) const {};
    virtual bool load(base::Stream* stream) {return true;};

    MediaH264DecoderPlugin() = default;
    virtual ~MediaH264DecoderPlugin() = default;

public:
    struct SnapshotState {
        std::vector<uint8_t> sps;  // sps NALU
        std::vector<uint8_t> pps;  // pps NALU
        std::vector<std::vector<uint8_t>> savedPackets;
        std::vector<uint8_t> savedDecodedFrame;  // only one or nothing

        void saveSps(std::vector<uint8_t> xsps) { sps = std::move(xsps); }

        void savePps(std::vector<uint8_t> xpps) { pps = std::move(xpps); }
        void savePacket(std::vector<uint8_t> xpacket) {
            savedPackets.push_back(std::move(xpacket));
        }

        void saveDecodedFrame(std::vector<uint8_t> frame) {
            savedDecodedFrame = std::move(frame);
        }

        void saveVec(base::Stream* stream,
                     const std::vector<uint8_t>& vec) const {
            stream->putBe32(vec.size());
            if (!vec.empty()) {
                stream->write(vec.data(), vec.size());
            }
        }
        void save(base::Stream* stream) const {
            saveVec(stream, sps);
            saveVec(stream, pps);
            stream->putBe32(savedPackets.size());
            for (size_t i = 0; i < savedPackets.size(); ++i) {
                saveVec(stream, savedPackets[i]);
            }
            saveVec(stream, savedDecodedFrame);
        }

        void loadVec(base::Stream* stream, std::vector<uint8_t>& vec) {
            int size = stream->getBe32();
            vec.resize(size);
            if (size > 0) {
                stream->read(vec.data(), size);
            }
        }
        void load(base::Stream* stream) {
            loadVec(stream, sps);
            loadVec(stream, pps);
            int count = stream->getBe32();
            savedPackets.resize(count);
            for (int i = 0; i < count; ++i) {
                loadVec(stream, savedPackets[i]);
            }
            loadVec(stream, savedDecodedFrame);
        }
    };
};

}  // namespace emulation
}  // namespace android
