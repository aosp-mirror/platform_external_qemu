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

#include "android/emulation/MediaVpxDecoder.h"
#include "android/base/system/System.h"
#include "android/emulation/MediaVpxDecoderGeneric.h"
#include "android/emulation/VpxPingInfoParser.h"

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#define MEDIA_VPX_DEBUG 0

#if MEDIA_VPX_DEBUG
#define VPX_DPRINT(fmt, ...)                                        \
    fprintf(stderr, "vpx-dec: %s:%d " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__);
#else
#define VPX_DPRINT(fmt,...)
#endif

namespace android {
namespace emulation {

namespace {
MediaVpxDecoderPlugin* makeDecoderPlugin(uint64_t pluginid,
                                         VpxPingInfoParser parser,
                                         MediaCodecType type) {
    return new MediaVpxDecoderGeneric(parser, type);
}
};  // namespace

uint64_t MediaVpxDecoder::readId(void* ptr) {
    if (nullptr == ptr)
        return 0;
    uint64_t key = VpxPingInfoParser::parseId(ptr);
    return key;
}

MediaVpxDecoderPlugin* MediaVpxDecoder::getDecoder(uint64_t key) {
    {
        std::lock_guard<std::mutex> g(mMapLock);
        auto iter = mDecoders.find(key);
        if (iter != mDecoders.end()) {
            return iter->second;
        }
    }
    VPX_DPRINT("Error: cannot find decoder with key %" PRIx64 "", key);
    return nullptr;
}

void MediaVpxDecoder::addDecoder(uint64_t key, MediaVpxDecoderPlugin* val) {
    {
        std::lock_guard<std::mutex> g(mMapLock);
        if (mDecoders.find(key) == mDecoders.end()) {
            mDecoders[key] = val;
            VPX_DPRINT("added decoder key %" PRIx64 " val: %p", key, val);
            return;
        }
    }
    VPX_DPRINT("cannot add: already exist");
}

void MediaVpxDecoder::removeDecoder(uint64_t key) {
    {
        std::lock_guard<std::mutex> g(mMapLock);
        auto iter = mDecoders.find(key);
        if (iter != mDecoders.end()) {
            VPX_DPRINT("removed decoder key %" PRIx64 ", val: %p", key,
                       mDecoders[key]);
            mDecoders.erase(iter);
            return;
        }
    }
    VPX_DPRINT("error: cannot remove decoder, not in map");
}

void MediaVpxDecoder::handlePing(MediaCodecType type,
                                 MediaOperation op,
                                 void* ptr) {
    using InitContextParam = VpxPingInfoParser::InitContextParam;
    using DecodeFrameParam = VpxPingInfoParser::DecodeFrameParam;
    using GetImageParam = VpxPingInfoParser::GetImageParam;

    switch (op) {
        case MediaOperation::InitContext: {
            VpxPingInfoParser parser{ptr};
            InitContextParam param{};
            parser.parseInitContextParams(ptr, param);
            VPX_DPRINT(
                    "handle init decoder context request from guest version %u",
                    parser.version());
            uint64_t myid = readId(ptr);
            MediaVpxDecoderPlugin* mydecoder =
                    makeDecoderPlugin(myid, parser, type);
            addDecoder(myid, mydecoder);
            mydecoder->initVpxContext(ptr);
            VPX_DPRINT("done handling InitContext");
            break;
        }
        case MediaOperation::DestroyContext: {
            VPX_DPRINT("handle destroy request from guest %p", ptr);
            MediaVpxDecoderPlugin* mydecoder = getDecoder(readId(ptr));
            if (!mydecoder)
                return;

            mydecoder->destroyVpxContext(ptr);
            delete mydecoder;
            removeDecoder(readId(ptr));
            break;
        }
        case MediaOperation::DecodeImage: {
            VPX_DPRINT("handle decodeimage request from guest %p", ptr);
            MediaVpxDecoderPlugin* mydecoder = getDecoder(readId(ptr));
            if (nullptr == mydecoder)
                return;
            mydecoder->decodeFrame(ptr);
            break;
        }
        case MediaOperation::Flush: {
            VPX_DPRINT("handle flush request from guest %p", ptr);
            MediaVpxDecoderPlugin* mydecoder = getDecoder(readId(ptr));
            if (nullptr == mydecoder)
                return;
            mydecoder->flush(ptr);
            break;
        }
        case MediaOperation::GetImage: {
            VPX_DPRINT("handle getimage request from guest %p", ptr);
            MediaVpxDecoderPlugin* mydecoder = getDecoder(readId(ptr));
            if (nullptr == mydecoder)
                return;
            mydecoder->getImage(ptr);
            break;
        }
        case MediaOperation::Reset: {
            VPX_DPRINT("Reset is not supported %u\n", (unsigned int)op);
        }
        default:
            VPX_DPRINT("Unknown command %u\n", (unsigned int)op);
            break;
    }
}

void MediaVpxDecoder::save(base::Stream* stream) const {
    int size = mDecoders.size();
    stream->putBe32(size);
    for (auto item : mDecoders) {
        stream->putBe64(item.first);
        stream->putBe32(item.second->type());
        stream->putBe32(item.second->vpxtype());
        stream->putBe32(item.second->version());
        item.second->save(stream);
    }
}

bool MediaVpxDecoder::load(base::Stream* stream) {
    VPX_DPRINT("loading ..");
    int size = stream->getBe32();
    for (int i = 0; i < size; ++i) {
        // this is hacky; but we have to know the plugin type
        uint64_t id = stream->getBe64();
        int type = stream->getBe32();
        MediaCodecType vpxtype = stream->getBe32() == 8
                                         ? MediaCodecType::VP8Codec
                                         : MediaCodecType::VP9Codec;
        int version = stream->getBe32();
        if (type == MediaVpxDecoderPlugin::PLUGIN_TYPE_GENERIC) {  // libvpx
            MediaVpxDecoderGeneric* decoder = new MediaVpxDecoderGeneric(
                    VpxPingInfoParser(version), vpxtype);
            mDecoders[id] = decoder;
            decoder->load(stream);
            continue;
        }
        fprintf(stderr, "Error, un-implemented %s %d\n", __func__, __LINE__);
        exit(1);
    }

    return true;
}

}  // namespace emulation
}  // namespace android
