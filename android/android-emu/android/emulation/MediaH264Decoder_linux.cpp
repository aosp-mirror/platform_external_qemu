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

#include "android/emulation/MediaH264Decoder.h"
#include "android/emulation/H264NaluParser.h"

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

// cuda and nvidia decoder related headers
#include <cuda.h>
#include <nvcuvid.h>

#define MEDIA_H264_DEBUG 1

#if MEDIA_H264_DEBUG
#define H264_DPRINT(fmt,...) fprintf(stderr, "h264-dec: %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define H264_DPRINT(fmt,...)
#endif

#define NVDEC_API_CALL( cuvidAPI )                                                                                 \
    do                                                                                                             \
    {                                                                                                              \
        CUresult errorCode = cuvidAPI;                                                                             \
        if( errorCode != CUDA_SUCCESS)                                                                             \
        {                                                                                                          \
            H264_DPRINT("%s failed with error code %d\n", #cuvidAPI, (int)errorCode);                              \
        }                                                                                                          \
    } while (0)

namespace android {
namespace emulation {

namespace {

class MediaH264DecoderImpl : public MediaH264Decoder {
public:
    MediaH264DecoderImpl() = default;
    virtual ~MediaH264DecoderImpl();

    // This is the entry point
    virtual void handlePing(MediaCodecType type, MediaOperation op, void* ptr) override;

private:
    virtual void initH264Context(unsigned int width,
                                 unsigned int height,
                                 unsigned int outWidth,
                                 unsigned int outHeight,
                                 PixelFormat pixFmt) override;
    virtual void destroyH264Context() override;
    virtual void decodeFrame(void* ptr, const uint8_t* frame, size_t szBytes) override;
    virtual void flush(void* ptr) override;
    virtual void getImage(void* ptr) override;

public:
    // cuda related methods
    static bool initCudaDrivers();
    static bool s_isCudaInitialized;

private:
    // cuda call back
    static int CUDAAPI HandleVideoSequenceProc(void *pUserData, CUVIDEOFORMAT *pVideoFormat) {
      return ((MediaH264DecoderImpl *)pUserData)->HandleVideoSequence(pVideoFormat);
    }
    static int CUDAAPI HandlePictureDecodeProc(void *pUserData, CUVIDPICPARAMS *pPicParams) {
      return ((MediaH264DecoderImpl *)pUserData)->HandlePictureDecode(pPicParams);
    }
    static int CUDAAPI HandlePictureDisplayProc(void *pUserData, CUVIDPARSERDISPINFO *pDispInfo) {
      return ((MediaH264DecoderImpl *)pUserData)->HandlePictureDisplay(pDispInfo);
    }

    int HandleVideoSequence(CUVIDEOFORMAT *pVideoFormat) {
      H264_DPRINT("NOT IMPLEMENTED");
      return CUDA_ERROR_NOT_SUPPORTED;
    }

    int HandlePictureDecode(CUVIDPICPARAMS *pPicParams) {
      H264_DPRINT("NOT IMPLEMENTED");
      return CUDA_ERROR_NOT_SUPPORTED;
    }

    int HandlePictureDisplay(CUVIDPARSERDISPINFO *pDispInfo) {
      H264_DPRINT("NOT IMPLEMENTED");
      return CUDA_ERROR_NOT_SUPPORTED;
    }

private:
    // image props
    bool mImageReady = false;
    static constexpr int kBPP = 2; // YUV420 is 2 bytes per pixel
    unsigned int mOutputHeight = 0;
    unsigned int mOutputWidth = 0;
    PixelFormat mOutPixFmt;
    unsigned int mOutBufferSize = 0;

    // cuda stuff
    CUcontext mCudaContext = nullptr;
    CUvideoctxlock mCtxLock;
    CUvideoparser mCudaParser = nullptr;
    CUvideodecoder mCudaDecoder = nullptr;

}; // MediaH264DecoderImpl

MediaH264DecoderImpl::~MediaH264DecoderImpl() {
}

void MediaH264DecoderImpl::handlePing(MediaCodecType type, MediaOperation op, void* ptr) {
    switch (op) {
        case MediaOperation::InitContext:
        {
            uint8_t* xptr = (uint8_t*)ptr;
            unsigned int width = *(unsigned int*)(xptr);
            unsigned int height = *(unsigned int*)(xptr + 8);
            unsigned int outWidth = *(unsigned int*)(xptr + 16);
            unsigned int outHeight = *(unsigned int*)(xptr + 24);
            PixelFormat pixFmt = static_cast<PixelFormat>(*(uint8_t*)(xptr + 30));
            initH264Context(width, height, outWidth, outHeight, pixFmt);
            break;
        }
        case MediaOperation::DestroyContext:
            destroyH264Context();
            break;
        case MediaOperation::DecodeImage:
        {
            uint64_t offset = *(uint64_t*)(ptr);
            const uint8_t* img = (const uint8_t*)ptr + offset;
            size_t szBytes = *(size_t*)((uint8_t*)ptr + 8);
            decodeFrame(ptr, img, szBytes);
            break;
        }
        case MediaOperation::Flush:
            flush(ptr);
            break;
        case MediaOperation::GetImage:
            getImage(ptr);
            break;
        default:
            H264_DPRINT("Unknown command %u\n", (unsigned int)op);
            break;
    }
}

void MediaH264DecoderImpl::initH264Context(unsigned int width,
                             unsigned int height,
                             unsigned int outWidth,
                             unsigned int outHeight,
                             PixelFormat outPixFmt) {
    H264_DPRINT("%s(w=%u h=%u out_w=%u out_h=%u pixfmt=%u)",
                __func__, width, height, outWidth, outHeight, (uint8_t)outPixFmt);
    mOutputWidth = outWidth;
    mOutputHeight = outHeight;
    mOutPixFmt = outPixFmt;
    mOutBufferSize = outWidth * outHeight * 3 / 2;

    // cudat stuff
    const int gpuIndex = 0;
    const int cudaFlags = 0;
    CUdevice cudaDevice = 0;
    CUresult myres = cuDeviceGet(&cudaDevice, gpuIndex);
    if (myres != CUDA_SUCCESS) {
      H264_DPRINT("Failed to get cuda device, error code %d", (int)myres);
      return;
    }

    char buf[1024];
    myres = cuDeviceGetName(buf, sizeof(buf), cudaDevice);
    if (myres != CUDA_SUCCESS) {
      H264_DPRINT("Failed to get gpu device name, error code %d", (int)myres);
      return;
    }

    H264_DPRINT("using gpu device %s", buf);

    myres = cuCtxCreate(&mCudaContext, cudaFlags, cudaDevice);
    if (myres != CUDA_SUCCESS) {
      H264_DPRINT("Failed to create cuda context, error code %d", (int)myres);
    }

    NVDEC_API_CALL(cuvidCtxLockCreate(&mCtxLock, mCudaContext));

    CUVIDPARSERPARAMS videoParserParameters = {};
    videoParserParameters.CodecType = cudaVideoCodec_H264;
    videoParserParameters.ulMaxNumDecodeSurfaces = 1;
    videoParserParameters.ulMaxDisplayDelay = 1;
    videoParserParameters.pUserData = this;
    videoParserParameters.pfnSequenceCallback = HandleVideoSequenceProc;
    videoParserParameters.pfnDecodePicture = HandlePictureDecodeProc;
    videoParserParameters.pfnDisplayPicture = HandlePictureDisplayProc;
    NVDEC_API_CALL(cuvidCreateVideoParser(&mCudaParser, &videoParserParameters));

    H264_DPRINT("Successfully created cuda context %p", mCudaContext);
}

void MediaH264DecoderImpl::destroyH264Context() {
    if (mCudaContext != nullptr) {
      CUresult myres = cuCtxDestroy(mCudaContext);
      if (myres != CUDA_SUCCESS) {
        H264_DPRINT("Failed to destroy cuda context; error code %d", (int)myres);
      }
      mCudaContext = nullptr;
    }
    if (mCudaParser != nullptr) {
      NVDEC_API_CALL(cuvidDestroyVideoParser(mCudaParser));
      mCudaParser = nullptr;
    }
}

void MediaH264DecoderImpl::decodeFrame(void* ptr, const uint8_t* frame, size_t szBytes) {
    H264_DPRINT("NOT IMPLEMENTED");
}

void MediaH264DecoderImpl::flush(void* ptr) {
    H264_DPRINT("NOT IMPLEMENTED");
}

void MediaH264DecoderImpl::getImage(void* ptr) {
    H264_DPRINT("NOT IMPLEMENTED");
}

bool MediaH264DecoderImpl::initCudaDrivers() {
    if (s_isCudaInitialized) {
      return true;
    }

    // this should be called at the very beginning, before we call anything else
    CUresult initResult = cuInit(0);
    if (initResult != CUDA_SUCCESS) {
        H264_DPRINT("Failed to init cuda drivers error code %d", (int)initResult);
        s_isCudaInitialized = false;
        return false;
    }

    int numGpuCards = 0;
    CUresult myres = cuDeviceGetCount(&numGpuCards);
    if (myres != CUDA_SUCCESS) {
        H264_DPRINT("Failed to get number of GPU cards installed on host; error code %d", (int)myres);
        return false;
    }

    if (numGpuCards <= 0) {
        H264_DPRINT("There are no nvidia GPU cards on this host.");
        return false;
    }

    // lukily, we get cuda initialized.
    s_isCudaInitialized = true;

    return true;
}

};  // namespace

bool MediaH264DecoderImpl::s_isCudaInitialized = false;
// static
MediaH264Decoder* MediaH264Decoder::create() {
    if(!MediaH264DecoderImpl::initCudaDrivers()) {
      return nullptr;
    }
    return new MediaH264DecoderImpl();
}

}  // namespace emulation
}  // namespace android

