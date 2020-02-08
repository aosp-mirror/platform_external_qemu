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

#include "android/emulation/MediaH264DecoderCuvid.h"
#include "android/emulation/H264NaluParser.h"
#include "android/emulation/YuvConverter.h"
// MediaH264DecoderCuvid.h
#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <winioctl.h>
#endif

#include <stdio.h>
#include <string.h>

extern "C" {
#include "android/emulation/dynlink_cuda.h"
#include "android/emulation/dynlink_nvcuvid.h"
}
#define MEDIA_H264_DEBUG 0

#if MEDIA_H264_DEBUG
#define H264_DPRINT(fmt, ...)                                              \
    fprintf(stderr, "h264-cuvid-dec: %s:%d " fmt "\n", __func__, __LINE__, \
            ##__VA_ARGS__);
#else
#define H264_DPRINT(fmt, ...)
#endif

#define NVDEC_API_CALL(cuvidAPI)                                     \
    do {                                                             \
        CUresult errorCode = cuvidAPI;                               \
        if (errorCode != CUDA_SUCCESS) {                             \
            H264_DPRINT("%s failed with error code %d\n", #cuvidAPI, \
                        (int)errorCode);                             \
        }                                                            \
    } while (0)

namespace android {
namespace emulation {

using InitContextParam = H264PingInfoParser::InitContextParam;
using DecodeFrameParam = H264PingInfoParser::DecodeFrameParam;
using ResetParam = H264PingInfoParser::ResetParam;
using GetImageParam = H264PingInfoParser::GetImageParam;
MediaH264DecoderCuvid::MediaH264DecoderCuvid(uint64_t id,
                                             H264PingInfoParser parser)
    : mId(id), mParser(parser){};

MediaH264DecoderPlugin* MediaH264DecoderCuvid::clone() {
    return new MediaH264DecoderCuvid(mId, mParser);
};

MediaH264DecoderCuvid::~MediaH264DecoderCuvid() {
    destroyH264Context();
}

void MediaH264DecoderCuvid::reset(void* ptr) {
    destroyH264Context();
    ResetParam param{};
    mParser.parseResetParams(ptr, param);
    initH264ContextInternal(param.width, param.height, param.outputWidth,
                            param.outputHeight, param.outputPixelFormat);
}

void MediaH264DecoderCuvid::initH264Context(void* ptr) {
    InitContextParam param{};
    mParser.parseInitContextParams(ptr, param);
    initH264ContextInternal(param.width, param.height, param.outputWidth,
                            param.outputHeight, param.outputPixelFormat);
}

void MediaH264DecoderCuvid::initH264ContextInternal(unsigned int width,
                                                    unsigned int height,
                                                    unsigned int outWidth,
                                                    unsigned int outHeight,
                                                    PixelFormat outPixFmt) {
    if (!initCudaDrivers()) {
        H264_DPRINT("Failed to initH264Context because driver is not working");
        return;
    }

    if (mCudaContext != nullptr) {
        destroyH264Context();
    }
    H264_DPRINT("%s(w=%u h=%u out_w=%u out_h=%u pixfmt=%u)", __func__, width,
                height, outWidth, outHeight, (uint8_t)outPixFmt);
    mWidth = width;
    mHeight = height;

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
    NVDEC_API_CALL(
            cuvidCreateVideoParser(&mCudaParser, &videoParserParameters));

    H264_DPRINT("Successfully created cuda context %p", mCudaContext);
}

void MediaH264DecoderCuvid::destroyH264Context() {
    H264_DPRINT("destroyH264Context calling");

    if (mCudaContext != nullptr) {
        NVDEC_API_CALL(cuCtxPushCurrent(mCudaContext));
        if (mCudaParser != nullptr) {
            NVDEC_API_CALL(cuvidDestroyVideoParser(mCudaParser));
            mCudaParser = nullptr;
        }

        if (mCudaDecoder != nullptr) {
            NVDEC_API_CALL(cuvidDestroyDecoder(mCudaDecoder));
            mCudaDecoder = nullptr;
        }
        NVDEC_API_CALL(cuCtxPopCurrent(NULL));
        NVDEC_API_CALL(cuvidCtxLockDestroy(mCtxLock));
    }

    if (mCudaContext != nullptr) {
        CUresult myres = cuCtxDestroy(mCudaContext);
        if (myres != CUDA_SUCCESS) {
            H264_DPRINT("Failed to destroy cuda context; error code %d",
                        (int)myres);
        }
        mCudaContext = nullptr;
    }
}

void MediaH264DecoderCuvid::decodeFrame(void* ptr) {
    DecodeFrameParam param{};
    mParser.parseDecodeFrameParams(ptr, param);

    const uint8_t* frame = param.pData;
    size_t szBytes = param.size;
    uint64_t inputPts = param.pts;

    mIsInFlush = false;
    static int numbers = 0;
    // H264_DPRINT("calling decodeframe %d for %d bytes", numbers++,
    // (int)szBytes);
    H264_DPRINT("%s(frame=%p, sz=%zu)", __func__, frame, szBytes);
    Err h264Err = Err::NoErr;
    // TODO: move this somewhere else
    // First return parameter is the number of bytes processed,
    // Second return parameter is the error code
    uint64_t* retSzBytes = param.pConsumedBytes;
    int32_t* retErr = param.pDecoderErrorCode;

    CUVIDSOURCEDATAPACKET packet = {0};
    packet.payload = frame;
    packet.payload_size = szBytes;
    packet.flags = CUVID_PKT_TIMESTAMP;
    packet.timestamp = inputPts;
    if (!frame || szBytes == 0) {
        packet.flags |= CUVID_PKT_ENDOFSTREAM;
    }
    NVDEC_API_CALL(cuvidParseVideoData(mCudaParser, &packet));
    *retSzBytes = szBytes;
    *retErr = (int32_t)h264Err;
}

void MediaH264DecoderCuvid::doFlush() {
    if (!mIsInFlush) {
        return;
    }
    H264_DPRINT("started flushing");
    CUVIDSOURCEDATAPACKET packet = {0};
    packet.payload = NULL;
    packet.payload_size = 0;
    packet.flags |= CUVID_PKT_ENDOFSTREAM;
    NVDEC_API_CALL(cuvidParseVideoData(mCudaParser, &packet));
    H264_DPRINT("done one flushing");
}

void MediaH264DecoderCuvid::flush(void* ptr) {
    mIsInFlush = true;
    doFlush();
}

void MediaH264DecoderCuvid::getImage(void* ptr) {
    H264_DPRINT("getImage %p", ptr);
    GetImageParam param{};
    mParser.parseGetImageParams(ptr, param);

    int* retErr = param.pDecoderErrorCode;
    uint32_t* retWidth = param.pRetWidth;
    uint32_t* retHeight = param.pRetHeight;
    uint32_t* retPts = param.pRetPts;
    uint32_t* retColorPrimaries = param.pRetColorPrimaries;
    uint32_t* retColorRange = param.pRetColorRange;
    uint32_t* retColorTransfer = param.pRetColorTransfer;
    uint32_t* retColorSpace = param.pRetColorSpace;

    static int numbers = 0;
    H264_DPRINT("calling getImage %d", numbers++);
    doFlush();
    uint8_t* dst = param.pDecodedFrame;
    int myOutputWidth = mOutputWidth;
    int myOutputHeight = mOutputHeight;
    {
        std::lock_guard<std::mutex> g(mFrameLock);
        mImageReady = !mSavedFrames.empty();
        if (!mImageReady) {
            H264_DPRINT("%s: no new frame yet", __func__);
            *retErr = static_cast<int>(Err::NoDecodedFrame);
            return;
        }
    std::vector<uint8_t>& myFrame = mSavedFrames.front();
    mOutputPts = mSavedPts.front();
    uint8_t* pDecodedFrame = &(myFrame[0]);

    int myOutputWidth = mSavedW.front();
    int myOutputHeight = mSavedH.front();
    *retWidth = myOutputWidth;
    *retHeight = myOutputHeight;

    memcpy(dst, pDecodedFrame, myOutputHeight * myOutputWidth * 3 / 2);
    mSavedFrames.pop_front();
    mSavedPts.pop_front();
    mSavedW.pop_front();
    mSavedH.pop_front();
    }

    YuvConverter<uint8_t> convert8(myOutputWidth, myOutputHeight);
    convert8.UVInterleavedToPlanar(dst);

    if (mParser.version() == 200) {
        mRenderer.renderToHostColorBuffer(param.hostColorBufferId,
                                          myOutputWidth, myOutputHeight, dst);
    }

    mImageReady = false;
    *retErr = myOutputHeight * myOutputWidth * 3 / 2;
    *retPts = mOutputPts;
    *retColorPrimaries = mColorPrimaries;
    *retColorRange = mColorRange;
    *retColorTransfer = mColorTransfer;
    *retColorSpace = mColorSpace;
    H264_DPRINT("Frame primary %d range %d transfer %d space %d",
                (int)mColorPrimaries, (int)mColorRange, (int)mColorTransfer,
                (int)mColorSpace);
    H264_DPRINT("Copying completed pts %lld", (long long)mOutputPts);
}

bool MediaH264DecoderCuvid::initCudaDrivers() {
    if (s_isCudaInitialized) {
        return true;
    }
#if defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64)
    typedef HMODULE CUDADRIVER;
#else
    typedef void* CUDADRIVER;
#endif
    CUDADRIVER hHandleDriver = 0;
    if (CUDA_SUCCESS != cuInit(0, __CUDA_API_VERSION, hHandleDriver)) {
        fprintf(stderr,
                "Failed to call cuInit, cannot use nvidia cuvid decoder for "
                "h264 stream\n");
        return false;
    }
    if (CUDA_SUCCESS != cuvidInit(0)) {
        fprintf(stderr,
                "Failed to call cuvidInit, cannot use nvidia cuvid decoder for "
                "h264 stream\n");
        return false;
    }

    int numGpuCards = 0;
    CUresult myres = cuDeviceGetCount(&numGpuCards);
    if (myres != CUDA_SUCCESS) {
        H264_DPRINT(
                "Failed to get number of GPU cards installed on host; error "
                "code %d",
                (int)myres);
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

int MediaH264DecoderCuvid::HandleVideoSequence(CUVIDEOFORMAT* pVideoFormat) {
    int nDecodeSurface = 4;  // pVideoFormat->min_num_decode_surfaces;

    CUVIDDECODECAPS decodecaps;
    memset(&decodecaps, 0, sizeof(decodecaps));

    decodecaps.eCodecType = pVideoFormat->codec;
    decodecaps.eChromaFormat = pVideoFormat->chroma_format;
    decodecaps.nBitDepthMinus8 = pVideoFormat->bit_depth_luma_minus8;

    NVDEC_API_CALL(cuCtxPushCurrent(mCudaContext));
    NVDEC_API_CALL(cuvidGetDecoderCaps(&decodecaps));
    NVDEC_API_CALL(cuCtxPopCurrent(NULL));

    if (!decodecaps.bIsSupported) {
        H264_DPRINT("Codec not supported on this GPU.");
        return nDecodeSurface;
    }

    if ((pVideoFormat->coded_width > decodecaps.nMaxWidth) ||
        (pVideoFormat->coded_height > decodecaps.nMaxHeight)) {
        H264_DPRINT("Resolution not supported on this GPU");
        return nDecodeSurface;
    }

    if ((pVideoFormat->coded_width >> 4) * (pVideoFormat->coded_height >> 4) >
        decodecaps.nMaxMBCount) {
        H264_DPRINT("MBCount not supported on this GPU");
        return nDecodeSurface;
    }

    mLumaWidth =
            pVideoFormat->display_area.right - pVideoFormat->display_area.left;
    mLumaHeight =
            pVideoFormat->display_area.bottom - pVideoFormat->display_area.top;
    mChromaHeight = mLumaHeight * 0.5;  // NV12
    mBPP = pVideoFormat->bit_depth_luma_minus8 > 0 ? 2 : 1;

    if (pVideoFormat->video_signal_description.video_full_range_flag)
        mColorRange = 2;
    else
        mColorRange = 0;

    mColorPrimaries = pVideoFormat->video_signal_description.color_primaries;
    mColorTransfer =
            pVideoFormat->video_signal_description.transfer_characteristics;
    mColorSpace = pVideoFormat->video_signal_description.matrix_coefficients;

    CUVIDDECODECREATEINFO videoDecodeCreateInfo = {0};
    videoDecodeCreateInfo.CodecType = pVideoFormat->codec;
    videoDecodeCreateInfo.ChromaFormat = pVideoFormat->chroma_format;
    videoDecodeCreateInfo.OutputFormat = cudaVideoSurfaceFormat_NV12;
    H264_DPRINT("output format is %d", videoDecodeCreateInfo.OutputFormat);
    videoDecodeCreateInfo.bitDepthMinus8 = pVideoFormat->bit_depth_luma_minus8;
    if (pVideoFormat->progressive_sequence)
        videoDecodeCreateInfo.DeinterlaceMode = cudaVideoDeinterlaceMode_Weave;
    else
        videoDecodeCreateInfo.DeinterlaceMode =
                cudaVideoDeinterlaceMode_Adaptive;
    videoDecodeCreateInfo.ulNumOutputSurfaces = 1;
    // With PreferCUVID, JPEG is still decoded by CUDA while video is decoded by
    // NVDEC hardware
    videoDecodeCreateInfo.ulCreationFlags = cudaVideoCreate_PreferCUVID;
    videoDecodeCreateInfo.ulNumDecodeSurfaces = nDecodeSurface;
    videoDecodeCreateInfo.vidLock = mCtxLock;
    videoDecodeCreateInfo.ulWidth = pVideoFormat->coded_width;
    videoDecodeCreateInfo.ulHeight = pVideoFormat->coded_height;
    if (mOutputHeight != mLumaHeight || mOutputWidth != mLumaWidth) {
        H264_DPRINT("old width %d old height %d", mOutputWidth, mOutputHeight);
        mOutputWidth = mLumaWidth;
        mOutputHeight = mLumaHeight;
        H264_DPRINT("new width %d new height %d", mOutputWidth, mOutputHeight);
        unsigned int newOutBufferSize = mOutputWidth * mOutputHeight * 3 / 2;
        if (mOutBufferSize < newOutBufferSize) {
            mOutBufferSize = newOutBufferSize;
        }
    }

    videoDecodeCreateInfo.ulTargetWidth = pVideoFormat->coded_width;
    videoDecodeCreateInfo.ulTargetHeight = pVideoFormat->coded_height;

    mSurfaceWidth = videoDecodeCreateInfo.ulTargetWidth;
    mSurfaceHeight = videoDecodeCreateInfo.ulTargetHeight;

    NVDEC_API_CALL(cuCtxPushCurrent(mCudaContext));
    if (mCudaDecoder != nullptr) {
        NVDEC_API_CALL(cuvidDestroyDecoder(mCudaDecoder));
        mCudaDecoder = nullptr;
    }
    {
        size_t free, total;
        cuMemGetInfo(&free, &total);
        H264_DPRINT("free memory %g M, total %g M", free / 1048576.0,
                    total / 1048576.0);
    }
    NVDEC_API_CALL(cuCtxPopCurrent(NULL));
    NVDEC_API_CALL(cuCtxPushCurrent(mCudaContext));
    NVDEC_API_CALL(cuvidCreateDecoder(&mCudaDecoder, &videoDecodeCreateInfo));
    NVDEC_API_CALL(cuCtxPopCurrent(NULL));
    H264_DPRINT("successfully called. decoder %p", mCudaDecoder);
    return nDecodeSurface;
}

int MediaH264DecoderCuvid::HandlePictureDecode(CUVIDPICPARAMS* pPicParams) {
    NVDEC_API_CALL(cuvidDecodePicture(mCudaDecoder, pPicParams));
    H264_DPRINT("successfully called.");
    return 1;
}

int MediaH264DecoderCuvid::HandlePictureDisplay(
        CUVIDPARSERDISPINFO* pDispInfo) {
    CUVIDPROCPARAMS videoProcessingParameters = {};
    videoProcessingParameters.progressive_frame = pDispInfo->progressive_frame;
    videoProcessingParameters.second_field = pDispInfo->repeat_first_field + 1;
    videoProcessingParameters.top_field_first = pDispInfo->top_field_first;
    videoProcessingParameters.unpaired_field =
            pDispInfo->repeat_first_field < 0;
    videoProcessingParameters.output_stream = 0;
    uint64_t myOutputPts = pDispInfo->timestamp;

    CUdeviceptr dpSrcFrame = 0;
    unsigned int nSrcPitch = 0;
    NVDEC_API_CALL(cuvidMapVideoFrame(mCudaDecoder, pDispInfo->picture_index,
                                      &dpSrcFrame, &nSrcPitch,
                                      &videoProcessingParameters));

    unsigned int newOutBufferSize = mOutputWidth * mOutputHeight * 3 / 2;
    std::vector<uint8_t> myFrame(newOutBufferSize);
    uint8_t* pDecodedFrame = &(myFrame[0]);

    NVDEC_API_CALL(cuCtxPushCurrent(mCudaContext));
    CUDA_MEMCPY2D m = {0};
    m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
    m.srcDevice = dpSrcFrame;
    m.srcPitch = nSrcPitch;
    m.dstMemoryType = CU_MEMORYTYPE_HOST;
    m.dstDevice = (CUdeviceptr)(m.dstHost = pDecodedFrame);
    m.dstPitch = mOutputWidth * mBPP;
    m.WidthInBytes = mOutputWidth * mBPP;
    m.Height = mLumaHeight;
    H264_DPRINT("dstDevice %p, dstPitch %d, WidthInBytes %d Height %d",
                m.dstHost, (int)m.dstPitch, (int)m.WidthInBytes, (int)m.Height);

    NVDEC_API_CALL(cuMemcpy2DAsync(&m, 0));

    m.srcDevice =
            (CUdeviceptr)((uint8_t*)dpSrcFrame + m.srcPitch * mSurfaceHeight);
    m.dstDevice =
            (CUdeviceptr)(m.dstHost = pDecodedFrame + m.dstPitch * mLumaHeight);
    m.Height = mChromaHeight;
    NVDEC_API_CALL(cuMemcpy2DAsync(&m, 0));

    NVDEC_API_CALL(cuStreamSynchronize(0));
    NVDEC_API_CALL(cuCtxPopCurrent(NULL));

    NVDEC_API_CALL(cuvidUnmapVideoFrame(mCudaDecoder, dpSrcFrame));
    {
        std::lock_guard<std::mutex> g(mFrameLock);
        mSavedFrames.push_back(myFrame);
        mSavedPts.push_back(myOutputPts);
        mSavedW.push_back(mOutputWidth);
        mSavedH.push_back(mOutputHeight);
    }
    mImageReady = true;
    H264_DPRINT("successfully called.");
    return 1;
}

bool MediaH264DecoderCuvid::s_isCudaInitialized = false;
// static

}  // namespace emulation
}  // namespace android
