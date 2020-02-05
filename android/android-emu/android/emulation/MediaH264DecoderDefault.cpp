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
#include "android/emulation/MediaH264DecoderFfmpeg.h"
#ifdef __APPLE__
#include "android/emulation/MediaH264DecoderVideoToolBoxProxy.h"
#else
// for Linux and Window, Cuvid is available
#include "android/emulation/MediaH264DecoderCuvid.h"
#endif
#include "android/base/system/System.h"

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
MediaH264DecoderPlugin* makeDecoderPlugin(uint32_t version) {
#ifdef __APPLE__
    auto useVideoToolBox = android::base::System::getEnvironmentVariable(
            "ANDROID_EMU_CODEC_USE_VIDEOTOOLBOX_DECODER");
    if (useVideoToolBox != "") {
        return new MediaH264DecoderVideoToolBoxProxy(version);
    }
#else
    auto useCuvidEnv = android::base::System::getEnvironmentVariable(
            "ANDROID_EMU_CODEC_USE_CUVID_DECODER");
    if (useCuvidEnv != "") {
        H264_DPRINT("Using Cuvid decoder on Linux/Windows");
        return new MediaH264DecoderCuvid(version);
    }
#endif
    return new MediaH264DecoderFfmpeg(version);
}

}; // anon namespace


uint64_t MediaH264DecoderDefault::readId(void* ptr) {
    if (nullptr == ptr)
        return 0;
    uint64_t key = (*reinterpret_cast<uint64_t*>(ptr));
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
    switch (op) {
        case MediaOperation::InitContext: {
            uint8_t* xptr = (uint8_t*)ptr;
            uint32_t version = *(uint32_t*)xptr;
            H264_DPRINT(
                    "handle init decoder context request from guest version %u",
                    version);
            unsigned int width = *(unsigned int*)(xptr + 8);
            unsigned int height = *(unsigned int*)(xptr + 16);
            unsigned int outWidth = *(unsigned int*)(xptr + 24);
            unsigned int outHeight = *(unsigned int*)(xptr + 32);
            PixelFormat pixFmt =
                    static_cast<PixelFormat>(*(uint8_t*)(xptr + 40));
            MediaH264DecoderPlugin* mydecoder = makeDecoderPlugin(version);
            uint64_t myid = createId();
            addDecoder(myid, mydecoder);
            mydecoder->initH264Context(width, height, outWidth, outHeight,
                                       pixFmt);
            uint64_t* ret = static_cast<uint64_t*>(getReturnAddress(ptr));
            *ret = myid;
            H264_DPRINT("done handling InitContext");
            break;
        }
        case MediaOperation::DestroyContext: {
            H264_DPRINT("handle destroy request from guest %p", ptr);
            MediaH264DecoderPlugin* mydecoder = getDecoder(readId(ptr));
            if (!mydecoder)
                return;

            mydecoder->destroyH264Context();
            delete mydecoder;
            removeDecoder(readId(ptr));
            break;
        }
        case MediaOperation::DecodeImage: {
            H264_DPRINT("handle decodeimage request from guest %p", ptr);
            MediaH264DecoderPlugin* mydecoder = getDecoder(readId(ptr));
            if (nullptr == mydecoder)
                return;
            uint64_t offset = *(uint64_t*)((uint8_t*)ptr + 8);
            const uint8_t* img = (const uint8_t*)ptr + offset;
            size_t szBytes = *(size_t*)((uint8_t*)ptr + 16);
            uint64_t inputPts = *(size_t*)((uint8_t*)ptr + 24);
            mydecoder->decodeFrame(ptr, img, szBytes, inputPts);
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
            uint8_t* xptr = (uint8_t*)ptr;
            unsigned int width = *(unsigned int*)(xptr + 8);
            unsigned int height = *(unsigned int*)(xptr + 16);
            unsigned int outWidth = *(unsigned int*)(xptr + 24);
            unsigned int outHeight = *(unsigned int*)(xptr + 32);
            PixelFormat pixFmt =
                    static_cast<PixelFormat>(*(uint8_t*)(xptr + 40));
            mydecoder->initH264Context(width, height, outWidth, outHeight,
                                       pixFmt);
            updateDecoder(oldId, mydecoder);
            break;
        }
        default:
            H264_DPRINT("Unknown command %u\n", (unsigned int)op);
            break;
    }
}

}  // namespace emulation
}  // namespace android

