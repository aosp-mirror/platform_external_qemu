// Copyright 2020 The Android Open Source Project
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
#include "android/emulation/MediaSnapshotState.h"

#include <stdio.h>
#include <cassert>

#define MEDIA_SNAPSTATE_DEBUG 0

#if MEDIA_SNAPSTATE_DEBUG
#define SNAPSTATE_DPRINT(fmt, ...)                                     \
    fprintf(stderr, "media-snapshot-state: %s:%d " fmt "\n", __func__, \
            __LINE__, ##__VA_ARGS__);
#else
#define SNAPSTATE_DPRINT(fmt, ...)
#endif

#include <string.h>

namespace android {
namespace emulation {

bool MediaSnapshotState::savePacket(std::vector<uint8_t> data, uint64_t pts) {
    if (pts > 0 && savedPackets.size() > 0 && pts == savedPackets.back().pts) {
        return false;
    }
    PacketInfo pkt{data, pts};
    savedPackets.push_back(std::move(pkt));
    return true;
}
bool MediaSnapshotState::savePacket(const uint8_t* frame,
                                    size_t size,
                                    uint64_t pts) {
    if (pts > 0 && savedPackets.size() > 0 && pts == savedPackets.back().pts) {
        return false;
    }
    std::vector<uint8_t> vec;
    vec.assign(frame, frame + size);
    PacketInfo pkt{vec, pts};
    savedPackets.push_back(std::move(pkt));
    return true;
}

void MediaSnapshotState::saveDecodedFrame(std::vector<uint8_t> data,
                                          int width,
                                          int height,
                                          uint64_t pts,
                                          ColorAspects xcolor) {
    SNAPSTATE_DPRINT("save decoded byte data");
    FrameInfo frame{
            std::move(data), std::vector<uint32_t>{}, width, height, pts,
            xcolor};
    savedFrames.push_back(std::move(frame));
}

void MediaSnapshotState::saveDecodedFrame(std::vector<uint32_t> texture,
                                          int width,
                                          int height,
                                          uint64_t pts,
                                          ColorAspects xcolor) {
    SNAPSTATE_DPRINT("save decoded texture");
    FrameInfo frame{std::vector<uint8_t>{},
                    std::move(texture),
                    width,
                    height,
                    pts,
                    xcolor};
    savedFrames.push_back(std::move(frame));
}

namespace {
template <class T>
void saveVec(base::Stream* stream, const std::vector<T>& vec) {
    stream->putBe32(vec.size());
    if (!vec.empty()) {
        stream->write(vec.data(), vec.size() * sizeof(vec[0]));
    }
}

template <class T>
void loadVec(base::Stream* stream, std::vector<T>& vec) {
    int size = stream->getBe32();
    vec.resize(size);
    if (size > 0) {
        stream->read(vec.data(), size * sizeof(vec[0]));
    }
}
}  // namespace

void MediaSnapshotState::saveFrameInfo(base::Stream* stream,
                                       const FrameInfo& frame) const {
    SNAPSTATE_DPRINT("save bytedata");
    saveVec(stream, frame.data);
    SNAPSTATE_DPRINT("save texture");
    saveVec(stream, frame.texture);
    stream->putBe32(frame.width);
    stream->putBe32(frame.height);
    saveColor(stream, frame.color);
    stream->putBe64(frame.pts);
}

void MediaSnapshotState::loadFrameInfo(base::Stream* stream, FrameInfo& frame) {
    SNAPSTATE_DPRINT("load bytedata");
    loadVec(stream, frame.data);
    SNAPSTATE_DPRINT("load texture");
    loadVec(stream, frame.texture);
    // set all to zero, as we dont really save texture
    std::fill(frame.texture.begin(), frame.texture.end(), 0);
    frame.width = stream->getBe32();
    frame.height = stream->getBe32();
    loadColor(stream, frame.color);
    frame.pts = stream->getBe64();
}

void MediaSnapshotState::savePacketInfo(base::Stream* stream,
                                        const PacketInfo& pkt) const {
    saveVec(stream, pkt.data);
    stream->putBe64(pkt.pts);
}

void MediaSnapshotState::loadPacketInfo(base::Stream* stream, PacketInfo& pkt) {
    loadVec(stream, pkt.data);
    pkt.pts = stream->getBe64();
}

void MediaSnapshotState::saveColor(base::Stream* stream,
                                   const ColorAspects& color) const {
    stream->putBe32(color.primaries);
    stream->putBe32(color.range);
    stream->putBe32(color.transfer);
    stream->putBe32(color.space);
}

void MediaSnapshotState::loadColor(base::Stream* stream,
                                   ColorAspects& color) const {
    color.primaries = stream->getBe32();
    color.range = stream->getBe32();
    color.transfer = stream->getBe32();
    color.space = stream->getBe32();
}

void MediaSnapshotState::save(base::Stream* stream) const {
    saveVec(stream, sps);
    saveVec(stream, pps);
    stream->putBe32(savedPackets.size());
    for (size_t i = 0; i < savedPackets.size(); ++i) {
        savePacketInfo(stream, savedPackets[i]);
    }
    stream->putBe32(savedFrames.size());
    SNAPSTATE_DPRINT("save now ");
    for (auto iter = savedFrames.begin(); iter != savedFrames.end(); ++iter) {
        SNAPSTATE_DPRINT("save now ");
        saveFrameInfo(stream, *iter);
    }
    // saveFrameInfo(stream, savedDecodedFrame);
}

void MediaSnapshotState::load(base::Stream* stream) {
    loadVec(stream, sps);
    loadVec(stream, pps);
    int count = stream->getBe32();
    savedPackets.resize(count);
    for (int i = 0; i < count; ++i) {
        loadPacketInfo(stream, savedPackets[i]);
    }
    int fcount = stream->getBe32();
    SNAPSTATE_DPRINT("load now ");
    for (int i = 0; i < fcount; ++i) {
        SNAPSTATE_DPRINT("load now ");
        loadFrameInfo(stream, savedDecodedFrame);
        savedFrames.push_back(std::move(savedDecodedFrame));
    }
}

}  // namespace emulation
}  // namespace android
