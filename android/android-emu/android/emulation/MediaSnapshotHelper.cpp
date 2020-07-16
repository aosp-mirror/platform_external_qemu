// Copyright (C) 2020 The Android Open Source Project
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

#include "android/emulation/MediaSnapshotHelper.h"
#include "android/emulation/H264NaluParser.h"
#include "android/emulation/VpxFrameParser.h"

#define MEDIA_SNAPSHOT_DEBUG 0

#if MEDIA_SNAPSHOT_DEBUG
#define SNAPSHOT_DPRINT(fmt, ...)                                       \
    fprintf(stderr, "media-snapshot-helper: %s:%d " fmt "\n", __func__, \
            __LINE__, ##__VA_ARGS__);
#else
#define SNAPSHOT_DPRINT(fmt, ...)
#endif

namespace android {
namespace emulation {

using PacketInfo = MediaSnapshotState::PacketInfo;
using ColorAspects = MediaSnapshotState::ColorAspects;

void MediaSnapshotHelper::savePacket(const uint8_t* frame,
                                     size_t szBytes,
                                     uint64_t inputPts) {
    if (mType == CodecType::H264) {
        return saveH264Packet(frame, szBytes, inputPts);
    } else {
        return saveVPXPacket(frame, szBytes, inputPts);
    }
}

void MediaSnapshotHelper::saveVPXPacket(const uint8_t* data,
                                        size_t len,
                                        uint64_t user_priv) {
    const bool enableSnapshot = true;
    if (enableSnapshot) {
        VpxFrameParser fparser(mType == CodecType::VP8 ? 8 : 9, data,
                               (size_t)len);
        SNAPSHOT_DPRINT("found frame type: %s frame",
                        fparser.isKeyFrame() ? "KEY" : "NON-KEY");
        std::vector<uint8_t> v;
        v.assign(data, data + len);
        bool isIFrame = fparser.isKeyFrame();
        if (isIFrame) {
            mSnapshotState.savedPackets.clear();
        }
        const bool saveOK = mSnapshotState.savePacket(v, user_priv);
        if (saveOK) {
            SNAPSHOT_DPRINT("saving packet of isze %d; total is %d",
                            (int)(v.size()),
                            (int)(mSnapshotState.savedPackets.size()));
        } else {
            SNAPSHOT_DPRINT("saving packet; has duplicate, skip; total is %d",
                            (int)(mSnapshotState.savedPackets.size()));
        }
    }
}

void MediaSnapshotHelper::saveH264Packet(const uint8_t* frame,
                                         size_t szBytes,
                                         uint64_t inputPts) {
    const bool enableSnapshot = true;
    if (enableSnapshot) {
        std::vector<uint8_t> v;
        v.assign(frame, frame + szBytes);
        bool hasSps = H264NaluParser::checkSpsFrame(frame, szBytes);
        if (hasSps) {
            SNAPSHOT_DPRINT("create new snapshot state");
            MediaSnapshotState newSnapshotState{};
            // we need to keep the frames, the guest might not have retrieved
            // them yet; otherwise, we might loose some frames
            std::swap(newSnapshotState.savedFrames, mSnapshotState.savedFrames);
            std::swap(newSnapshotState, mSnapshotState);
            mSnapshotState.saveSps(v);
        } else {
            bool hasPps = H264NaluParser::checkPpsFrame(frame, szBytes);
            if (hasPps) {
                mSnapshotState.savePps(v);
                mSnapshotState.savedPackets.clear();
                mSnapshotState.savedDecodedFrame.data.clear();
            } else {
                bool isIFrame = H264NaluParser::checkIFrame(frame, szBytes);
                if (isIFrame) {
                    mSnapshotState.savedPackets.clear();
                }
                mSnapshotState.savePacket(std::move(v), inputPts);
                SNAPSHOT_DPRINT("saving packet; total is %d",
                                (int)(mSnapshotState.savedPackets.size()));
            }
        }
    }
}
void MediaSnapshotHelper::save(base::Stream* stream) const {
    SNAPSHOT_DPRINT("saving packets now %d",
                    (int)(mSnapshotState.savedPackets.size()));
    if (mType == CodecType::H264) {
        stream->putBe32(264);
    } else if (mType == CodecType::VP8) {
        stream->putBe32(8);
    } else if (mType == CodecType::VP9) {
        stream->putBe32(9);
    }
    mSnapshotState.save(stream);
}

void MediaSnapshotHelper::replay(
        std::function<void(uint8_t* data, size_t len, uint64_t pts)>
                oneShotDecode) {
    if (mSnapshotState.sps.size() > 0) {
        oneShotDecode(mSnapshotState.sps.data(), mSnapshotState.sps.size(), 0);
        if (mSnapshotState.pps.size() > 0) {
            oneShotDecode(mSnapshotState.pps.data(), mSnapshotState.pps.size(),
                          0);
            for (int i = 0; i < mSnapshotState.savedPackets.size(); ++i) {
                MediaSnapshotState::PacketInfo& pkt =
                        mSnapshotState.savedPackets[i];
                SNAPSHOT_DPRINT("reloading frame %d size %d", i,
                                (int)pkt.data.size());
                oneShotDecode(pkt.data.data(), pkt.data.size(), pkt.pts);
            }
        }
    }
}

void MediaSnapshotHelper::load(
        base::Stream* stream,
        std::function<void(uint8_t* data, size_t len, uint64_t pts)>
                oneShotDecode) {
    int type = stream->getBe32();
    if (type == 264) {
        mType = CodecType::H264;
    } else if (type == 8) {
        mType = CodecType::VP8;
    } else if (type == 9) {
        mType = CodecType::VP9;
    }

    mSnapshotState.load(stream);

    SNAPSHOT_DPRINT("loaded packets %d, now restore decoder",
                    (int)(mSnapshotState.savedPackets.size()));

    replay(oneShotDecode);

    SNAPSHOT_DPRINT("Done loading snapshots frames\n\n");
}

}  // namespace emulation
}  // namespace android
