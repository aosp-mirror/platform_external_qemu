// Copyright (C) 2022 The Android Open Source Project
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

#include "android/emulation/MediaHevcDecoderDefault.h"
#include "android/base/system/System.h"
#include "android/emulation/HevcPingInfoParser.h"
#include "android/emulation/MediaHevcDecoderGeneric.h"

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#define MEDIA_HEVC_DEBUG 0

#if MEDIA_HEVC_DEBUG
#define HEVC_DPRINT(fmt, ...)                                        \
    fprintf(stderr, "hevc-dec: %s:%d " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__);
#else
#define HEVC_DPRINT(fmt, ...)
#endif

namespace android {
namespace emulation {

namespace {
MediaHevcDecoderPlugin* makeDecoderPlugin(uint64_t pluginid,
                                          HevcPingInfoParser parser) {
    return new MediaHevcDecoderGeneric(pluginid, parser);
}

};  // namespace

uint64_t MediaHevcDecoderDefault::readId(void* ptr) {
    if (nullptr == ptr)
        return 0;
    uint64_t key = HevcPingInfoParser::parseHostDecoderId(ptr);
    return key;
}

MediaHevcDecoderPlugin* MediaHevcDecoderDefault::getDecoder(uint64_t key) {
    {
        std::lock_guard<std::mutex> g(mMapLock);
        auto iter = mDecoders.find(key);
        if (iter != mDecoders.end()) {
            return iter->second;
        }
    }
    HEVC_DPRINT("Error: cannot find decoder with key %" PRIx64 "", key);
    return nullptr;
}

uint64_t MediaHevcDecoderDefault::createId() {
    std::lock_guard<std::mutex> g(mIdLock);
    return ++mId;
}

void MediaHevcDecoderDefault::addDecoder(uint64_t key,
                                         MediaHevcDecoderPlugin* val) {
    {
        std::lock_guard<std::mutex> g(mMapLock);
        if (mDecoders.find(key) == mDecoders.end()) {
            mDecoders[key] = val;
            HEVC_DPRINT("added decoder key %" PRIx64 " val: %p", key, val);
            return;
        }
    }
    HEVC_DPRINT("cannot add: already exist");
}

void MediaHevcDecoderDefault::updateDecoder(uint64_t key,
                                            MediaHevcDecoderPlugin* val) {
    std::lock_guard<std::mutex> g(mMapLock);
    if (mDecoders.find(key) == mDecoders.end()) {
        HEVC_DPRINT("error: decoder with key %" PRIx64 " does not exist", key);
    } else {
        mDecoders[key] = val;
        HEVC_DPRINT("updated key %" PRIx64 " with new decoder %p", key, val);
    }
}

void MediaHevcDecoderDefault::removeDecoder(uint64_t key) {
    {
        std::lock_guard<std::mutex> g(mMapLock);
        auto iter = mDecoders.find(key);
        if (iter != mDecoders.end()) {
            HEVC_DPRINT("removed decoder key %" PRIx64 ", val: %p", key,
                        mDecoders[key]);
            mDecoders.erase(iter);
            return;
        }
    }
    HEVC_DPRINT("error: cannot remove decoder, not in map");
}

static void* getReturnAddress(void* ptr) {
    uint8_t* xptr = (uint8_t*)ptr;
    void* pint = (void*)(xptr + 256);
    return pint;
}

void MediaHevcDecoderDefault::handlePing(MediaCodecType type,
                                         MediaOperation op,
                                         void* ptr) {
    using InitContextParam = HevcPingInfoParser::InitContextParam;
    using DecodeFrameParam = HevcPingInfoParser::DecodeFrameParam;
    using ResetParam = HevcPingInfoParser::ResetParam;
    using GetImageParam = HevcPingInfoParser::GetImageParam;

    switch (op) {
        case MediaOperation::InitContext: {
            HevcPingInfoParser parser{ptr};
            InitContextParam param{};
            parser.parseInitContextParams(ptr, param);
            HEVC_DPRINT(
                    "handle init decoder context request from guest version %u",
                    parser.version());
            uint64_t myid = createId();
            MediaHevcDecoderPlugin* mydecoder = makeDecoderPlugin(myid, parser);
            addDecoder(myid, mydecoder);
            mydecoder->initHevcContext(ptr);
            *(param.pHostDecoderId) = myid;
            HEVC_DPRINT("done handling InitContext");
            break;
        }
        case MediaOperation::DestroyContext: {
            HEVC_DPRINT("handle destroy request from guest %p", ptr);
            MediaHevcDecoderPlugin* mydecoder = getDecoder(readId(ptr));
            if (!mydecoder)
                return;

            delete mydecoder;
            removeDecoder(readId(ptr));
            break;
        }
        case MediaOperation::DecodeImage: {
            HEVC_DPRINT("handle decodeimage request from guest %p", ptr);
            MediaHevcDecoderPlugin* mydecoder = getDecoder(readId(ptr));
            if (nullptr == mydecoder)
                return;
            mydecoder->decodeFrame(ptr);
            break;
        }
        case MediaOperation::Flush: {
            HEVC_DPRINT("handle flush request from guest %p", ptr);
            MediaHevcDecoderPlugin* mydecoder = getDecoder(readId(ptr));
            if (nullptr == mydecoder)
                return;
            mydecoder->flush(ptr);
            break;
        }
        case MediaOperation::GetImage: {
            HEVC_DPRINT("handle getimage request from guest %p", ptr);
            MediaHevcDecoderPlugin* mydecoder = getDecoder(readId(ptr));
            if (nullptr == mydecoder)
                return;
            mydecoder->getImage(ptr);
            break;
        }
        case MediaOperation::SendMetadata: {
            HEVC_DPRINT("handle send metadata request from guest %p", ptr);
            MediaHevcDecoderPlugin* mydecoder = getDecoder(readId(ptr));
            if (nullptr == mydecoder)
                return;
            mydecoder->sendMetadata(ptr);
            break;
        }
        case MediaOperation::Reset: {
            HEVC_DPRINT("handle reset request from guest %p", ptr);
            uint64_t oldId = readId(ptr);
            MediaHevcDecoderPlugin* olddecoder = getDecoder(oldId);
            if (nullptr == olddecoder) {
                HEVC_DPRINT("error, cannot reset on nullptr");
                return;
            }
            MediaHevcDecoderPlugin* mydecoder = olddecoder->clone();
            delete olddecoder;
            mydecoder->reset(ptr);
            updateDecoder(oldId, mydecoder);
            break;
        }
        default:
            HEVC_DPRINT("Unknown command %u\n", (unsigned int)op);
            break;
    }
}

void MediaHevcDecoderDefault::save(base::Stream* stream) const {
    stream->putBe64(mId);
    int size = mDecoders.size();
    stream->putBe32(size);
    for (auto item : mDecoders) {
        stream->putBe64(item.first);
        stream->putBe32(item.second->type());
        item.second->save(stream);
    }
}

bool MediaHevcDecoderDefault::load(base::Stream* stream) {
    mId = stream->getBe64();
    int size = stream->getBe32();
    for (int i = 0; i < size; ++i) {
        // this is hacky; but we have to know the plugin type
        uint64_t id = stream->getBe64();
        int type = stream->getBe32();
        if (type == MediaHevcDecoderPlugin::PLUGIN_TYPE_GENERIC) {
            MediaHevcDecoderGeneric* decoder =
                    new MediaHevcDecoderGeneric(id, HevcPingInfoParser(100));
            decoder->load(stream);
            mDecoders[id] = decoder;
            continue;
        }
        fprintf(stderr, "Error, un-implemented %s %d\n", __func__, __LINE__);
        exit(1);
    }
    return true;
}

}  // namespace emulation
}  // namespace android
