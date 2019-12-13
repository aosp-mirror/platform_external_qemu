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
#include "android/emulation/MediaH264DecoderDefault.h"
#include "android/emulation/H264NaluParser.h"
#include "android/emulation/CudaVideoLibraryLoader.h"

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
            const char *msg = NULL;                              \
            s_func->cuGetErrorName(errorCode, &msg);                              \
            H264_DPRINT("%s failed with error code %d %s\n", #cuvidAPI, (int)errorCode, msg);                              \
        }                                                                                                          \
    } while (0)

namespace android {
namespace emulation {

namespace {

template<typename T>
class YuvConverter {
public:
    YuvConverter(int nWidth, int nHeight) : nWidth(nWidth), nHeight(nHeight) {
        pQuad = new T[nWidth * nHeight / 4];
    }
    ~YuvConverter() {
        delete pQuad;
    }
    void PlanarToUVInterleaved(T *pFrame, int nPitch = 0) {
        if (nPitch == 0) {
            nPitch = nWidth;
        }
        T *puv = pFrame + nPitch * nHeight;
        if (nPitch == nWidth) {
            memcpy(pQuad, puv, nWidth * nHeight / 4 * sizeof(T));
        } else {
            for (int i = 0; i < nHeight / 2; i++) {
                memcpy(pQuad + nWidth / 2 * i, puv + nPitch / 2 * i, nWidth / 2 * sizeof(T));
            }
        }
        T *pv = puv + (nPitch / 2) * (nHeight / 2);
        for (int y = 0; y < nHeight / 2; y++) {
            for (int x = 0; x < nWidth / 2; x++) {
                puv[y * nPitch + x * 2] = pQuad[y * nWidth / 2 + x];
                puv[y * nPitch + x * 2 + 1] = pv[y * nPitch / 2 + x];
            }
        }
    }
    void UVInterleavedToPlanar(T *pFrame, int nPitch = 0) {
        if (nPitch == 0) {
            nPitch = nWidth;
        }
        T *puv = pFrame + nPitch * nHeight,
            *pu = puv,
            *pv = puv + nPitch * nHeight / 4;
        for (int y = 0; y < nHeight / 2; y++) {
            for (int x = 0; x < nWidth / 2; x++) {
                pu[y * nPitch / 2 + x] = puv[y * nPitch + x * 2];
                pQuad[y * nWidth / 2 + x] = puv[y * nPitch + x * 2 + 1];
            }
        }
        if (nPitch == nWidth) {
            memcpy(pv, pQuad, nWidth * nHeight / 4 * sizeof(T));
        } else {
            for (int i = 0; i < nHeight / 2; i++) {
                memcpy(pv + nPitch / 2 * i, pQuad + nWidth / 2 * i, nWidth / 2 * sizeof(T));
            }
        }
    }

private:
    T *pQuad;
    int nWidth, nHeight;
};

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
    static CudaVideoLibraryLoader s_myloader;
    static CudaVideoFunctions *s_func;

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
    unsigned int mBPP = 0;
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
    CUresult myres = s_func->cuDeviceGet(&cudaDevice, gpuIndex);
    if (myres != CUDA_SUCCESS) {
      H264_DPRINT("Failed to get cuda device, error code %d", (int)myres);
      return;
    }

    char buf[1024];
    myres = s_func->cuDeviceGetName(buf, sizeof(buf), cudaDevice);
    if (myres != CUDA_SUCCESS) {
      H264_DPRINT("Failed to get gpu device name, error code %d", (int)myres);
      return;
    }

    H264_DPRINT("using gpu device %s", buf);

    myres = s_func->cuCtxCreate(&mCudaContext, cudaFlags, cudaDevice);
    if (myres != CUDA_SUCCESS) {
      H264_DPRINT("Failed to create cuda context, error code %d", (int)myres);
    } else {
      H264_DPRINT("created cuda context %p", mCudaContext);
    }

    NVDEC_API_CALL(s_func->cuvidCtxLockCreate(&mCtxLock, mCudaContext));

    CUVIDPARSERPARAMS videoParserParameters = {};
    videoParserParameters.CodecType = cudaVideoCodec_H264;
    videoParserParameters.ulMaxNumDecodeSurfaces = 1;
    videoParserParameters.ulMaxDisplayDelay = 1;
    videoParserParameters.pUserData = this;
    videoParserParameters.pfnSequenceCallback = HandleVideoSequenceProc;
    videoParserParameters.pfnDecodePicture = HandlePictureDecodeProc;
    videoParserParameters.pfnDisplayPicture = HandlePictureDisplayProc;
    NVDEC_API_CALL(s_func->cuvidCreateVideoParser(&mCudaParser, &videoParserParameters));

    H264_DPRINT("Successfully created cuda context %p parser %p", mCudaContext, mCudaParser);
}

void MediaH264DecoderImpl::destroyH264Context() {

    if (mDecodedFrame) {
      //delete [] mDecodedFrame;
      mDecodedFrame = nullptr;
    }

    NVDEC_API_CALL(s_func->cuCtxPushCurrent(mCudaContext));
    NVDEC_API_CALL(s_func->cuCtxPopCurrent(NULL));
    if (mCudaParser != nullptr) {
      NVDEC_API_CALL(s_func->cuvidDestroyVideoParser(mCudaParser));
      mCudaParser = nullptr;
    }

    if (mCudaDecoder != nullptr) {
        NVDEC_API_CALL(s_func->cuvidDestroyDecoder(mCudaDecoder));
        mCudaDecoder = nullptr;
    }

    NVDEC_API_CALL(s_func->cuvidCtxLockDestroy(mCtxLock));

    if (mCudaContext != nullptr) {
      CUresult myres = s_func->cuCtxDestroy(mCudaContext);
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
    H264_DPRINT("now calling cuvidParseVideoData with %p with function %p", mCudaParser,
                s_func->cuvidParseVideoData);
    s_myloader.print(s_func);
    NVDEC_API_CALL(s_func->cuvidParseVideoData(mCudaParser, &packet));
    H264_DPRINT("successfully called cuvidParseVideoData");
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
    YuvConverter<uint8_t> convert8(mOutputWidth, mOutputHeight);
    convert8.UVInterleavedToPlanar(dst);
    mImageReady = false;
    *retErr = mOutBufferSize;
}

bool MediaH264DecoderImpl::initCudaDrivers() {
    if (s_isCudaInitialized) {
      return true;
    }

    // load up cuda library
    CudaVideoLibraryLoader s_myloader;
    bool success = s_myloader.openLibrary();
    if (!success) {
        H264_DPRINT("cannot open cuda so library");
        exit(1);
    } else {
        H264_DPRINT("succesfully opened cuda so library");
    }

    CudaVideoFunctions* pf = s_myloader.getFunctions();
    if (!pf) {
        H264_DPRINT("empty cuda functions");
        exit(1);
    } else {
        *s_func = *pf;
        H264_DPRINT("successfully got cuda functions");
    }

    // this should be called at the very beginning, before we call anything else
    if (s_func->cuInit == nullptr) {
        H264_DPRINT("empty cuInit");
        exit(1);
    }

    CUresult initResult = s_func->cuInit(0);
    if (initResult != CUDA_SUCCESS) {
        H264_DPRINT("Failed to init cuda drivers error code %d", (int)initResult);
        s_isCudaInitialized = false;
        return false;
    } else {
        H264_DPRINT("successfully called cuInit");
    }

    int numGpuCards = 0;
    CUresult myres = s_func->cuDeviceGetCount(&numGpuCards);
    if (myres != CUDA_SUCCESS) {
        H264_DPRINT("Failed to get number of GPU cards installed on host; error code %d", (int)myres);
        return false;
    } else {
        H264_DPRINT("successfully called cuDeviceGetCount");
    }

    if (numGpuCards <= 0) {
        H264_DPRINT("There are no nvidia GPU cards on this host.");
        return false;
    }

    s_myloader.print(s_func);
    // lukily, we get cuda initialized.
    s_isCudaInitialized = true;

    return true;
}

int MediaH264DecoderImpl::HandleVideoSequence(CUVIDEOFORMAT *pVideoFormat) {

    H264_DPRINT("calling handle-video sequence with context %p", mCudaContext);
    int nDecodeSurface = pVideoFormat->min_num_decode_surfaces;

    CUVIDDECODECAPS decodecaps;
    memset(&decodecaps, 0, sizeof(decodecaps));

    decodecaps.eCodecType = pVideoFormat->codec;
    decodecaps.eChromaFormat = pVideoFormat->chroma_format;
    decodecaps.nBitDepthMinus8 = pVideoFormat->bit_depth_luma_minus8;

    H264_DPRINT("calling handle-video sequence");
    NVDEC_API_CALL(s_func->cuCtxPushCurrent(mCudaContext));
    H264_DPRINT("calling handle-video sequence");
    NVDEC_API_CALL(s_func->cuvidGetDecoderCaps(&decodecaps));
    H264_DPRINT("calling handle-video sequence");
    NVDEC_API_CALL(s_func->cuCtxPopCurrent(NULL));

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
    mBPP = pVideoFormat->bit_depth_luma_minus8 > 0 ? 2 : 1;

    CUVIDDECODECREATEINFO videoDecodeCreateInfo = { 0 };
    videoDecodeCreateInfo.CodecType = pVideoFormat->codec;
    videoDecodeCreateInfo.ChromaFormat = pVideoFormat->chroma_format;
    videoDecodeCreateInfo.OutputFormat = cudaVideoSurfaceFormat_NV12;
    H264_DPRINT("output format is %d", videoDecodeCreateInfo.OutputFormat);
    videoDecodeCreateInfo.bitDepthMinus8 = pVideoFormat->bit_depth_luma_minus8;
    if (pVideoFormat->progressive_sequence)
        videoDecodeCreateInfo.DeinterlaceMode = cudaVideoDeinterlaceMode_Weave;
    else
        videoDecodeCreateInfo.DeinterlaceMode = cudaVideoDeinterlaceMode_Adaptive;
    videoDecodeCreateInfo.ulNumOutputSurfaces = 1;
    // With PreferCUVID, JPEG is still decoded by CUDA while video is decoded by NVDEC hardware
    videoDecodeCreateInfo.ulCreationFlags = cudaVideoCreate_PreferCUVID;
    videoDecodeCreateInfo.ulNumDecodeSurfaces = nDecodeSurface;
    videoDecodeCreateInfo.vidLock = mCtxLock;
    videoDecodeCreateInfo.ulWidth = pVideoFormat->coded_width;
    videoDecodeCreateInfo.ulHeight= pVideoFormat->coded_height;
    videoDecodeCreateInfo.ulMaxWidth = pVideoFormat->coded_width;
    videoDecodeCreateInfo.ulMaxHeight = pVideoFormat->coded_height;

    videoDecodeCreateInfo.ulTargetWidth = pVideoFormat->coded_width;
    videoDecodeCreateInfo.ulTargetHeight = pVideoFormat->coded_height;

    mSurfaceWidth = videoDecodeCreateInfo.ulTargetWidth;
    mSurfaceHeight = videoDecodeCreateInfo.ulTargetHeight;

    H264_DPRINT("calling handle-video sequence with cudacontext %p", mCudaContext);
    NVDEC_API_CALL(s_func->cuCtxPushCurrent(mCudaContext));
    if (mCudaDecoder != nullptr) {
        NVDEC_API_CALL(s_func->cuvidDestroyDecoder(mCudaDecoder));
        mCudaDecoder = nullptr;
    }
    {
      unsigned int free, total;
      s_func->cuMemGetInfo(&free, &total);
      H264_DPRINT("free memory %g M, total %g M", free/1048576.0, total/1048576.0);
    }
    H264_DPRINT("calling handle-video sequence");
    NVDEC_API_CALL(s_func->cuCtxPopCurrent(NULL));
    H264_DPRINT("calling handle-video sequence");
    NVDEC_API_CALL(s_func->cuCtxPushCurrent(mCudaContext));
    H264_DPRINT("calling handle-video sequence");
    NVDEC_API_CALL(s_func->cuvidCreateDecoder(&mCudaDecoder, &videoDecodeCreateInfo));
    H264_DPRINT("calling handle-video sequence");
    NVDEC_API_CALL(s_func->cuCtxPopCurrent(NULL));
    H264_DPRINT("calling handle-video sequence");
    H264_DPRINT("successfully called. decoder %p", mCudaDecoder);
    return nDecodeSurface;
}

int MediaH264DecoderImpl::HandlePictureDecode(CUVIDPICPARAMS *pPicParams) {
    NVDEC_API_CALL(s_func->cuvidDecodePicture(mCudaDecoder, pPicParams));
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
    (s_func->cuvidMapVideoFrame(mCudaDecoder, pDispInfo->picture_index, (unsigned int*)&dpSrcFrame, &nSrcPitch, &videoProcessingParameters));

    CUVIDGETDECODESTATUS DecodeStatus;
    memset(&DecodeStatus, 0, sizeof(DecodeStatus));
    CUresult result = s_func->cuvidGetDecodeStatus(mCudaDecoder, pDispInfo->picture_index, &DecodeStatus);
    if (result == CUDA_SUCCESS && (DecodeStatus.decodeStatus == cuvidDecodeStatus_Error
                                   || DecodeStatus.decodeStatus == cuvidDecodeStatus_Error_Concealed))
    {
        H264_DPRINT("Decode Error occurred for picture");
        return 0;
    }

    uint8_t *pDecodedFrame = mDecodedFrame;

    NVDEC_API_CALL(s_func->cuCtxPushCurrent(mCudaContext));
    CUDA_MEMCPY2D m = { 0 };
    m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
    m.srcDevice = dpSrcFrame;
    m.srcPitch = nSrcPitch;
    m.dstMemoryType = CU_MEMORYTYPE_HOST;
    m.dstDevice = (CUdeviceptr)(m.dstHost = pDecodedFrame);
    m.dstPitch = mOutputWidth* mBPP;
    m.WidthInBytes = mOutputWidth* mBPP;
    m.Height = mLumaHeight;
    H264_DPRINT("dstDevice %p, dstPitch %d, WidthInBytes %d Height %d",
                m.dstHost, (int)m.dstPitch, (int)m.WidthInBytes, (int)m.Height);

    NVDEC_API_CALL(s_func->cuMemcpy2DAsync(&m, 0));

    m.srcDevice = (CUdeviceptr)((uint8_t *)dpSrcFrame + m.srcPitch * mSurfaceHeight);
    m.dstDevice = (CUdeviceptr)(m.dstHost = pDecodedFrame + m.dstPitch * mLumaHeight);
    m.Height = mChromaHeight;
    NVDEC_API_CALL(s_func->cuMemcpy2DAsync(&m, 0));

    NVDEC_API_CALL(s_func->cuStreamSynchronize(0));
    NVDEC_API_CALL(s_func->cuCtxPopCurrent(NULL));

    NVDEC_API_CALL(s_func->cuvidUnmapVideoFrame(mCudaDecoder, dpSrcFrame));
    mImageReady = true;
    s_myloader.print(s_func);
    H264_DPRINT("successfully called.");
    return 1;
}
};  // namespace

bool MediaH264DecoderImpl::s_isCudaInitialized = false;
CudaVideoLibraryLoader MediaH264DecoderImpl::s_myloader;
CudaVideoFunctions* MediaH264DecoderImpl::s_func = new CudaVideoFunctions();
// static
MediaH264Decoder* MediaH264Decoder::create() {
    if(!MediaH264DecoderImpl::initCudaDrivers()) {
      return new MediaH264DecoderDefault();
    }
    return new MediaH264DecoderImpl();
}

}  // namespace emulation
}  // namespace android

