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

#include "android/emulation/MediaH264DecoderDefault.h"
#include "android/base/system/System.h"
#include "android/emulation/H264PingInfoParser.h"
#include "android/emulation/MediaH264DecoderGeneric.h"

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#define MEDIA_H264_DEBUG 0

#if MEDIA_H264_DEBUG
#define H264_DPRINT(fmt,...) fprintf(stderr, "h264-dec: %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define H264_DPRINT(fmt,...)
#endif

namespace android {
namespace emulation {

namespace {
MediaH264DecoderPlugin* makeDecoderPlugin(uint64_t pluginid,
                                          H264PingInfoParser parser) {
    return new MediaH264DecoderGeneric(pluginid, parser);
}

}; // anon namespace


uint64_t MediaH264DecoderDefault::readId(void* ptr) {
    if (nullptr == ptr)
        return 0;
    uint64_t key = H264PingInfoParser::parseHostDecoderId(ptr);
    return key;
}

MediaH264DecoderPlugin* MediaH264DecoderDefault::getDecoder(uint64_t key) {
    {
        std::lock_guard<std::mutex> g(mMapLock);
        auto iter = mDecoders.find(key);
        if (iter != mDecoders.end()) {
            return iter->second;
        }
    }
    H264_DPRINT("Error: cannot find decoder with key %" PRIx64 "", key);
    return nullptr;
}

uint64_t MediaH264DecoderDefault::createId() {
    std::lock_guard<std::mutex> g(mIdLock);
    return ++mId;
}

void MediaH264DecoderDefault::addDecoder(uint64_t key,
                                         MediaH264DecoderPlugin* val) {
    {
        std::lock_guard<std::mutex> g(mMapLock);
        if (mDecoders.find(key) == mDecoders.end()) {
            mDecoders[key] = val;
            H264_DPRINT("added decoder key %" PRIx64 " val: %p", key, val);
            return;
        }
    }
    H264_DPRINT("cannot add: already exist");
}

void MediaH264DecoderDefault::updateDecoder(uint64_t key,
                                            MediaH264DecoderPlugin* val) {
    std::lock_guard<std::mutex> g(mMapLock);
    if (mDecoders.find(key) == mDecoders.end()) {
        H264_DPRINT("error: decoder with key %" PRIx64 " does not exist", key);
    } else {
        mDecoders[key] = val;
        H264_DPRINT("updated key %" PRIx64 " with new decoder %p", key, val);
    }
}

void MediaH264DecoderDefault::removeDecoder(uint64_t key) {
    {
        std::lock_guard<std::mutex> g(mMapLock);
        auto iter = mDecoders.find(key);
        if (iter != mDecoders.end()) {
            H264_DPRINT("removed decoder key %" PRIx64 ", val: %p", key,
                        mDecoders[key]);
            mDecoders.erase(iter);
            return;
        }
    }
    H264_DPRINT("error: cannot remove decoder, not in map");
}

static void* getReturnAddress(void* ptr) {
    uint8_t* xptr = (uint8_t*)ptr;
    void* pint = (void*)(xptr + 256);
    return pint;
}


void MediaH264DecoderDefault::handlePing(MediaCodecType type,
                                         MediaOperation op,
                                         void* ptr) {
    using InitContextParam = H264PingInfoParser::InitContextParam;
    using DecodeFrameParam = H264PingInfoParser::DecodeFrameParam;
    using ResetParam = H264PingInfoParser::ResetParam;
    using GetImageParam = H264PingInfoParser::GetImageParam;

    switch (op) {
        case MediaOperation::InitContext: {
            H264PingInfoParser parser{ptr};
            InitContextParam param{};
            parser.parseInitContextParams(ptr, param);
            H264_DPRINT(
                    "handle init decoder context request from guest version %u",
                    parser.version());
            uint64_t myid = createId();
            MediaH264DecoderPlugin* mydecoder = makeDecoderPlugin(myid, parser);
            addDecoder(myid, mydecoder);
            mydecoder->initH264Context(ptr);
            *(param.pHostDecoderId) = myid;
            H264_DPRINT("done handling InitContext");
            break;
        }
        case MediaOperation::DestroyContext: {
            H264_DPRINT("handle destroy request from guest %p", ptr);
            MediaH264DecoderPlugin* mydecoder = getDecoder(readId(ptr));
            if (!mydecoder)
                return;

            delete mydecoder;
            removeDecoder(readId(ptr));
            break;
        }
        case MediaOperation::DecodeImage: {
            H264_DPRINT("handle decodeimage request from guest %p", ptr);
            MediaH264DecoderPlugin* mydecoder = getDecoder(readId(ptr));
            if (nullptr == mydecoder)
                return;
            mydecoder->decodeFrame(ptr);
            break;
        }
        case MediaOperation::Flush: {
            H264_DPRINT("handle flush request from guest %p", ptr);
            MediaH264DecoderPlugin* mydecoder = getDecoder(readId(ptr));
            if (nullptr == mydecoder)
                return;
            mydecoder->flush(ptr);
            break;
        }
        case MediaOperation::GetImage: {
            H264_DPRINT("handle getimage request from guest %p", ptr);
            MediaH264DecoderPlugin* mydecoder = getDecoder(readId(ptr));
            if (nullptr == mydecoder)
                return;
            mydecoder->getImage(ptr);
            break;
        }
        case MediaOperation::Reset: {
            H264_DPRINT("handle reset request from guest %p", ptr);
            uint64_t oldId = readId(ptr);
            MediaH264DecoderPlugin* olddecoder = getDecoder(oldId);
            if (nullptr == olddecoder) {
                H264_DPRINT("error, cannot reset on nullptr");
                return;
            }
            MediaH264DecoderPlugin* mydecoder = olddecoder->clone();
            delete olddecoder;
            mydecoder->reset(ptr);
            updateDecoder(oldId, mydecoder);
            break;
        }
        default:
            H264_DPRINT("Unknown command %u\n", (unsigned int)op);
            break;
    }
}

void MediaH264DecoderDefault::save(base::Stream* stream) const {
    stream->putBe64(mId);
    int size = mDecoders.size();
    stream->putBe32(size);
    for (auto item : mDecoders) {
        stream->putBe64(item.first);
        stream->putBe32(item.second->type());
        item.second->save(stream);
    }
}

bool MediaH264DecoderDefault::load(base::Stream* stream) {
    mId = stream->getBe64();
    int size = stream->getBe32();
    for (int i = 0; i < size; ++i) {
        // this is hacky; but we have to know the plugin type
        uint64_t id = stream->getBe64();
        int type = stream->getBe32();
        if (type == MediaH264DecoderPlugin::PLUGIN_TYPE_GENERIC) {
            MediaH264DecoderGeneric* decoder =
                    new MediaH264DecoderGeneric(id, H264PingInfoParser(100));
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

