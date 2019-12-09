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

    int HandleVideoSequence(CUVIDEOFORMAT *pVideoFormat);

    int HandlePictureDecode(CUVIDPICPARAMS *pPicParams);

    int HandlePictureDisplay(CUVIDPARSERDISPINFO *pDispInfo);

private:
    // image props
    bool mImageReady = false;
    static constexpr int kBPP = 2; // YUV420 is 2 bytes per pixel
    unsigned int mOutputHeight = 0;
    unsigned int mOutputWidth = 0;
    unsigned int mSurfaceHeight = 0;
    unsigned int mBPP = kBPP;
    unsigned int mSurfaceWidth = 0;
    unsigned int mLumaHeight = 0;
    unsigned int mChromaHeight = 0;
    PixelFormat mOutPixFmt;
    unsigned int mOutBufferSize = 0;

    // right now, decoding command only passes the input address;
    // and output address is only available in getImage().
    // TODO: this should be set to the output address to avoid
    // extra copying
    uint8_t *mDecodedFrame = nullptr;


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
    if (mCudaContext != nullptr) {
      destroyH264Context();
    }
    H264_DPRINT("%s(w=%u h=%u out_w=%u out_h=%u pixfmt=%u)",
                __func__, width, height, outWidth, outHeight, (uint8_t)outPixFmt);
    mOutputWidth = outWidth;
    mOutputHeight = outHeight;
    mOutPixFmt = outPixFmt;
    mOutBufferSize = outWidth * outHeight * 3 / 2;

    if (mDecodedFrame) {
      delete [] mDecodedFrame;
    }

    mDecodedFrame = new uint8_t[mOutBufferSize];

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

    if (mDecodedFrame) {
      //delete [] mDecodedFrame;
      mDecodedFrame = nullptr;
    }

    NVDEC_API_CALL(cuCtxPushCurrent(mCudaContext));
    NVDEC_API_CALL(cuCtxPopCurrent(NULL));
    if (mCudaParser != nullptr) {
      NVDEC_API_CALL(cuvidDestroyVideoParser(mCudaParser));
      mCudaParser = nullptr;
    }

    if (mCudaDecoder != nullptr) {
        NVDEC_API_CALL(cuvidDestroyDecoder(mCudaDecoder));
        mCudaDecoder = nullptr;
    }

    NVDEC_API_CALL(cuvidCtxLockDestroy(mCtxLock));

    if (mCudaContext != nullptr) {
      CUresult myres = cuCtxDestroy(mCudaContext);
      if (myres != CUDA_SUCCESS) {
        H264_DPRINT("Failed to destroy cuda context; error code %d", (int)myres);
      }
      mCudaContext = nullptr;
    }
}

static void* getReturnAddress(void* ptr) {
    uint8_t* xptr = (uint8_t*)ptr;
    void* pint = (void*)(xptr + 256);
    return pint;
}

void MediaH264DecoderImpl::decodeFrame(void* ptr, const uint8_t* frame, size_t szBytes) {
    static int numbers=0;
    //H264_DPRINT("calling decodeframe %d for %d bytes", numbers++, (int)szBytes);
    H264_DPRINT("%s(frame=%p, sz=%zu)", __func__, frame, szBytes);
    Err h264Err = Err::NoErr;
    // TODO: move this somewhere else
    // First return parameter is the number of bytes processed,
    // Second return parameter is the error code
    uint8_t* retptr = (uint8_t*)getReturnAddress(ptr);
    uint64_t* retSzBytes = (uint64_t*)retptr;
    int32_t* retErr = (int32_t*)(retptr + 8);

    CUVIDSOURCEDATAPACKET packet = {0};
    packet.payload = frame;
    packet.payload_size = szBytes;
    packet.flags = CUVID_PKT_TIMESTAMP;
    const int timestamp = 0;
    packet.timestamp = timestamp;
    if (!frame || szBytes == 0) {
        packet.flags |= CUVID_PKT_ENDOFSTREAM;
    }
    NVDEC_API_CALL(cuvidParseVideoData(mCudaParser, &packet));
    *retSzBytes = szBytes;
    *retErr = (int32_t)h264Err;
}

void MediaH264DecoderImpl::flush(void* ptr) {
    H264_DPRINT("NOT IMPLEMENTED");
}

static uint8_t* getDst(void* ptr) {
    // Guest will pass us the offset from the start address for where to write the image data.
    uint8_t* xptr = (uint8_t*)ptr;
    uint64_t offset = *(uint64_t*)(xptr);
    return (uint8_t*)ptr + offset;
}

void MediaH264DecoderImpl::getImage(void* ptr) {
    uint8_t* retptr = (uint8_t*)getReturnAddress(ptr);
    int* retErr = (int*)(retptr);
    uint32_t* retWidth = (uint32_t*)(retptr + 8);
    uint32_t* retHeight = (uint32_t*)(retptr + 16);

    static int numbers=0;
    //H264_DPRINT("calling getImage %d", numbers++);
    if (!mDecodedFrame) {
        H264_DPRINT("%s: frame is null", __func__);
        *retErr = static_cast<int>(Err::NoDecodedFrame);
        return;
    }
    if (!mImageReady) {
        //H264_DPRINT("%s: no new frame yet", __func__);
        *retErr = static_cast<int>(Err::NoDecodedFrame);
        return;
    }

    *retWidth = mOutputWidth;
    *retHeight = mOutputHeight;

    uint8_t* dst =  getDst(ptr);
    memcpy(dst, mDecodedFrame, mOutBufferSize);
    mImageReady = false;
    *retErr = mOutBufferSize;
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

int MediaH264DecoderImpl::HandleVideoSequence(CUVIDEOFORMAT *pVideoFormat) {

    int nDecodeSurface = pVideoFormat->min_num_decode_surfaces;

    CUVIDDECODECAPS decodecaps;
    memset(&decodecaps, 0, sizeof(decodecaps));

    decodecaps.eCodecType = pVideoFormat->codec;
    decodecaps.eChromaFormat = pVideoFormat->chroma_format;
    decodecaps.nBitDepthMinus8 = pVideoFormat->bit_depth_luma_minus8;

    NVDEC_API_CALL(cuCtxPushCurrent(mCudaContext));
    NVDEC_API_CALL(cuvidGetDecoderCaps(&decodecaps));
    NVDEC_API_CALL(cuCtxPopCurrent(NULL));

    if(!decodecaps.bIsSupported){
        H264_DPRINT("Codec not supported on this GPU.");
        return nDecodeSurface;
    }

    if ((pVideoFormat->coded_width > decodecaps.nMaxWidth) ||
        (pVideoFormat->coded_height > decodecaps.nMaxHeight)){
        H264_DPRINT("Resolution not supported on this GPU");
        return nDecodeSurface;
    }

    if ((pVideoFormat->coded_width>>4)*(pVideoFormat->coded_height>>4) > decodecaps.nMaxMBCount){
        H264_DPRINT("MBCount not supported on this GPU");
        return nDecodeSurface;
    }

    mLumaHeight = pVideoFormat->display_area.bottom - pVideoFormat->display_area.top;
    mChromaHeight = mLumaHeight * 0.5; // NV12

    CUVIDDECODECREATEINFO videoDecodeCreateInfo = { 0 };
    videoDecodeCreateInfo.CodecType = pVideoFormat->codec;
    videoDecodeCreateInfo.ChromaFormat = pVideoFormat->chroma_format;
    videoDecodeCreateInfo.OutputFormat = cudaVideoSurfaceFormat_NV12;
    videoDecodeCreateInfo.bitDepthMinus8 = pVideoFormat->bit_depth_luma_minus8;
    if (pVideoFormat->progressive_sequence)
        videoDecodeCreateInfo.DeinterlaceMode = cudaVideoDeinterlaceMode_Weave;
    else
        videoDecodeCreateInfo.DeinterlaceMode = cudaVideoDeinterlaceMode_Adaptive;
    videoDecodeCreateInfo.ulNumOutputSurfaces = 2;
    // With PreferCUVID, JPEG is still decoded by CUDA while video is decoded by NVDEC hardware
    videoDecodeCreateInfo.ulCreationFlags = cudaVideoCreate_PreferCUVID;
    videoDecodeCreateInfo.ulNumDecodeSurfaces = nDecodeSurface;
    videoDecodeCreateInfo.vidLock = mCtxLock;
    videoDecodeCreateInfo.ulWidth = pVideoFormat->coded_width;
    videoDecodeCreateInfo.ulMaxWidth = pVideoFormat->coded_width;
    videoDecodeCreateInfo.ulMaxHeight = pVideoFormat->coded_height;

    videoDecodeCreateInfo.ulTargetWidth = pVideoFormat->coded_width;
    videoDecodeCreateInfo.ulTargetHeight = pVideoFormat->coded_height;

    mSurfaceWidth = videoDecodeCreateInfo.ulTargetWidth;
    mSurfaceHeight = videoDecodeCreateInfo.ulTargetHeight;

    NVDEC_API_CALL(cuCtxPushCurrent(mCudaContext));
    if (mCudaDecoder != nullptr) {
        NVDEC_API_CALL(cuvidDestroyDecoder(mCudaDecoder));
        mCudaDecoder = nullptr;
    }
    NVDEC_API_CALL(cuvidCreateDecoder(&mCudaDecoder, &videoDecodeCreateInfo));
    NVDEC_API_CALL(cuCtxPopCurrent(NULL));
    H264_DPRINT("successfully called.");
    return nDecodeSurface;
}

int MediaH264DecoderImpl::HandlePictureDecode(CUVIDPICPARAMS *pPicParams) {
    NVDEC_API_CALL(cuvidDecodePicture(mCudaDecoder, pPicParams));
    H264_DPRINT("successfully called.");
    return 1;
}

int MediaH264DecoderImpl::HandlePictureDisplay(CUVIDPARSERDISPINFO *pDispInfo) {
    CUVIDPROCPARAMS videoProcessingParameters = {};
    videoProcessingParameters.progressive_frame = pDispInfo->progressive_frame;
    videoProcessingParameters.second_field = pDispInfo->repeat_first_field + 1;
    videoProcessingParameters.top_field_first = pDispInfo->top_field_first;
    videoProcessingParameters.unpaired_field = pDispInfo->repeat_first_field < 0;
    videoProcessingParameters.output_stream = 0;

    CUdeviceptr dpSrcFrame = 0;
    unsigned int nSrcPitch = 0;
    NVDEC_API_CALL(cuvidMapVideoFrame(mCudaDecoder, pDispInfo->picture_index, &dpSrcFrame,
        &nSrcPitch, &videoProcessingParameters));

    CUVIDGETDECODESTATUS DecodeStatus;
    memset(&DecodeStatus, 0, sizeof(DecodeStatus));
    CUresult result = cuvidGetDecodeStatus(mCudaDecoder, pDispInfo->picture_index, &DecodeStatus);
    if (result == CUDA_SUCCESS && (DecodeStatus.decodeStatus == cuvidDecodeStatus_Error
                                   || DecodeStatus.decodeStatus == cuvidDecodeStatus_Error_Concealed))
    {
        H264_DPRINT("Decode Error occurred for picture");
        return 0;
    }

    uint8_t *pDecodedFrame = mDecodedFrame;

    NVDEC_API_CALL(cuCtxPushCurrent(mCudaContext));
    CUDA_MEMCPY2D m = { 0 };
    m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
    m.srcDevice = dpSrcFrame;
    m.srcPitch = nSrcPitch;
    m.dstMemoryType = CU_MEMORYTYPE_HOST;
    m.dstDevice = (CUdeviceptr)(m.dstHost = pDecodedFrame);
    m.dstPitch = mOutputWidth* mBPP;
    m.WidthInBytes = mOutputWidth* mBPP;
    m.Height = mLumaHeight;
    NVDEC_API_CALL(cuMemcpy2DAsync(&m, 0));

    m.srcDevice = (CUdeviceptr)((uint8_t *)dpSrcFrame + m.srcPitch * mSurfaceHeight);
    m.dstDevice = (CUdeviceptr)(m.dstHost = pDecodedFrame + m.dstPitch * mLumaHeight);
    m.Height = mChromaHeight;
    NVDEC_API_CALL(cuMemcpy2DAsync(&m, 0));

    NVDEC_API_CALL(cuStreamSynchronize(0));
    NVDEC_API_CALL(cuCtxPopCurrent(NULL));

    NVDEC_API_CALL(cuvidUnmapVideoFrame(mCudaDecoder, dpSrcFrame));
    mImageReady = true;
    H264_DPRINT("successfully called.");
    return 1;
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

