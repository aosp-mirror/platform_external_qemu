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

class MediaH264DecoderDefault;

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

protected:
    // solely for snapshot save load purpose
    enum {
        PLUGIN_TYPE_NONE = 0,
        PLUGIN_TYPE_FFMPEG = 1,
        PLUGIN_TYPE_CUVID = 2,
        PLUGIN_TYPE_VIDEO_TOOL_BOX = 3,
        PLUGIN_TYPE_VIDEO_TOOL_BOX_PROXY = 4,
        PLUGIN_TYPE_GENERIC = 5,

    };

    // this is required by save/load
    virtual int type() const { return PLUGIN_TYPE_NONE; }

    friend MediaH264DecoderDefault;

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
        int width;
        int height;
        ColorAspects color;
        uint64_t pts;
    };

    struct SnapshotState {
        std::vector<uint8_t> sps;  // sps NALU
        std::vector<uint8_t> pps;  // pps NALU
        std::vector<PacketInfo> savedPackets;
        FrameInfo savedDecodedFrame;  // only one or nothing
        std::vector<FrameInfo> savedFrames;

        void saveSps(std::vector<uint8_t> xsps) { sps = std::move(xsps); }

        void savePps(std::vector<uint8_t> xpps) { pps = std::move(xpps); }
        bool savePacket(std::vector<uint8_t> data, uint64_t pts = 0) {
            if (pts > 0 && savedPackets.size() > 0 &&
                pts == savedPackets.back().pts) {
                return false;
            }
            PacketInfo pkt{data, pts};
            savedPackets.push_back(std::move(pkt));
            return true;
        }
        bool savePacket(const uint8_t* frame, size_t size, uint64_t pts = 0) {
            if (pts > 0 && savedPackets.size() > 0 &&
                pts == savedPackets.back().pts) {
                return false;
            }
            std::vector<uint8_t> vec;
            vec.assign(frame, frame+size);
            PacketInfo pkt{vec, pts};
            savedPackets.push_back(std::move(pkt));
            return true;
        }

        void saveDecodedFrame(std::vector<uint8_t> data,
                              int width = 0,
                              int height = 0,
                              ColorAspects xcolor = ColorAspects{},
                              uint64_t pts = 0) {
            FrameInfo frame{data, width, height, xcolor, pts};
            savedDecodedFrame = std::move(frame);
            savedFrames.push_back(savedDecodedFrame);
        }

        void saveVec(base::Stream* stream,
                     const std::vector<uint8_t>& vec) const {
            stream->putBe32(vec.size());
            if (!vec.empty()) {
                stream->write(vec.data(), vec.size());
            }
        }

        void saveFrameInfo(base::Stream* stream, const FrameInfo& frame) const {
            saveVec(stream, frame.data);
            stream->putBe32(frame.width);
            stream->putBe32(frame.height);
            saveColor(stream, frame.color);
            stream->putBe64(frame.pts);
        }

        void loadFrameInfo(base::Stream* stream, FrameInfo& frame) {
            loadVec(stream, frame.data);
            frame.width = stream->getBe32();
            frame.height = stream->getBe32();
            loadColor(stream, frame.color);
            frame.pts = stream->getBe64();
        }

        void savePacketInfo(base::Stream* stream, const PacketInfo& pkt) const {
            saveVec(stream, pkt.data);
            stream->putBe64(pkt.pts);
        }

        void loadPacketInfo(base::Stream* stream, PacketInfo& pkt) {
            loadVec(stream, pkt.data);
            pkt.pts = stream->getBe64();
        }

        void saveColor(base::Stream* stream, const ColorAspects& color) const {
            stream->putBe32(color.primaries);
            stream->putBe32(color.range);
            stream->putBe32(color.transfer);
            stream->putBe32(color.space);
        }

        void loadColor(base::Stream* stream, ColorAspects& color) const {
            color.primaries = stream->getBe32();
            color.range = stream->getBe32();
            color.transfer = stream->getBe32();
            color.space = stream->getBe32();
        }

        void save(base::Stream* stream) const {
            saveVec(stream, sps);
            saveVec(stream, pps);
            stream->putBe32(savedPackets.size());
            for (size_t i = 0; i < savedPackets.size(); ++i) {
                savePacketInfo(stream, savedPackets[i]);
            }
            saveFrameInfo(stream, savedDecodedFrame);
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
                loadPacketInfo(stream, savedPackets[i]);
            }
            loadFrameInfo(stream, savedDecodedFrame);
        }
    };
};

}  // namespace emulation
}  // namespace android
