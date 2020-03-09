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

#include "android/utils/system.h"

// MediaH264DecoderCuvid.h
#include <cstdint>
#include <string>
#include <chrono>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#include <winioctl.h>
#endif

#include <stdio.h>
#include <string.h>

#define INIT_CUDA_GL 1

extern "C" {
#include "android/emulation/dynlink_cuda.h"
#include "android/emulation/dynlink_cudaGL.h"
#include "android/emulation/dynlink_nvcuvid.h"
}
#define MEDIA_H264_DEBUG 1

#if MEDIA_H264_DEBUG
#define H264_DPRINT(fmt, ...)                                              \
    fprintf(stderr, "h264-cuvid-dec: %s:%d tid: %ld " fmt "\n", __func__, __LINE__, android_get_thread_id(), \
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

static CUcontext g_mCudaContext = nullptr;
static CUcontext g_mGLCudaContext = nullptr;
static unsigned int g_nSrcPitch = 0;
static CUdeviceptr g_dpSrcFrame = 0;
static unsigned int g_mSurfaceHeight = 0;
static int g_hostColorBufferId = 0;
static int g_width = 0;
static int g_height = 0;

static bool g_gpuCopy = true;

extern "C" {
void cuda_copy_decoded_frame(void* src_frame, uint32_t dest_texture_handle, int mode, int width, int height);
void cuda_nv12_updater(uint32_t Ytex, uint32_t UVtext);
}

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

    //myres = cuGLCtxCreate(&mCudaContext, cudaFlags, cudaDevice);
    myres = cuCtxCreate(&mCudaContext, cudaFlags, cudaDevice);
    if (myres != CUDA_SUCCESS) {
        H264_DPRINT("Failed to create cuda context, error code %d", (int)myres);
    }

    NVDEC_API_CALL(cuvidCtxLockCreate(&mCtxLock, mCudaContext));

    CUVIDPARSERPARAMS videoParserParameters = {};
    videoParserParameters.CodecType = cudaVideoCodec_H264;
    videoParserParameters.ulMaxNumDecodeSurfaces = 1;
    videoParserParameters.ulMaxDisplayDelay = 4;
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
    auto startTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::milliseconds::zero();

    DecodeFrameParam param{};
    mParser.parseDecodeFrameParams(ptr, param);

    const uint8_t* frame = param.pData;
    size_t szBytes = param.size;
    uint64_t inputPts = param.pts;

    const bool enableSnapshot = false;
    if (enableSnapshot) {
        std::vector<uint8_t> v;
        v.assign(frame, frame + szBytes);
        bool hasSps = H264NaluParser::checkSpsFrame(frame, szBytes);
        if (hasSps) {
            mSnapshotState = SnapshotState{};
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
                H264_DPRINT("saving packet; total is %d",
                            (int)(mSnapshotState.savedPackets.size()));
            }
        }
    }

    decodeFrameInternal(param.pConsumedBytes, param.pDecoderErrorCode, frame,
                        szBytes, inputPts, param.hostColorBufferId);
     elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime);

     H264_DPRINT("decoding takes %lld ms", elapsed.count());

}


void MediaH264DecoderCuvid::decodeFrameInternal(uint64_t* pRetSzBytes,
                                                int32_t* pRetErr,
                                                const uint8_t* frame,
                                                size_t szBytes,
                                                uint64_t inputPts, int hostColorBufferId) {
    auto startTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::milliseconds::zero();
    mIsInFlush = false;
    H264_DPRINT("%s frame=%p, sz=%zu pts %lu hostcolorbufferid %d $$$$$$$$$$", __func__, frame, szBytes, inputPts, hostColorBufferId);
    Err h264Err = Err::NoErr;

    g_hostColorBufferId = hostColorBufferId;

    CUVIDSOURCEDATAPACKET packet = {0};
    packet.payload = frame;
    packet.payload_size = szBytes;
    packet.flags = CUVID_PKT_TIMESTAMP;
    packet.timestamp = inputPts;
    if (!frame || szBytes == 0) {
        packet.flags |= CUVID_PKT_ENDOFSTREAM;
    }
    NVDEC_API_CALL(cuvidParseVideoData(mCudaParser, &packet));
    if (pRetSzBytes) {
        *pRetSzBytes = szBytes;
    }
    if (pRetErr) {
        *pRetErr = (int32_t)h264Err;
    }

     elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime);
     H264_DPRINT("decoding takes %lld ms", elapsed.count());
    H264_DPRINT("done with %s %d ************\n\n", __func__, __LINE__);
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
    auto startTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::milliseconds::zero();
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
    H264_DPRINT("calling getImage %d colorbuffer %d", numbers++, param.hostColorBufferId);
    g_hostColorBufferId = param.hostColorBufferId;
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

    YuvConverter<uint8_t> convert8(myOutputWidth, myOutputHeight);
     elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime);
     H264_DPRINT("get image takes %lld ms\n\n", elapsed.count());
    if (mParser.version() == 200) {
        if (param.hostColorBufferId >= 0 && g_gpuCopy) {
            MediaHostRenderer::TextureFrame texFrame = mSavedTexFrames.front();
            mSavedTexFrames.pop_front();
            mRenderer.renderToHostColorBufferWithTextures(
                    g_hostColorBufferId, mOutputWidth, mOutputHeight, texFrame);
            // mRenderer.renderToHostColorBufferWithCallback(g_hostColorBufferId,
            // mOutputWidth,
            //                              mOutputHeight, pDecodedFrame,
            //                              cuda_copy_decoded_frame);
            // convert8.UVInterleavedToPlanar(pDecodedFrame);
            // memset(pDecodedFrame, 0x0f, myOutputHeight * myOutputWidth * 3 /
            // 2); mRenderer.renderToHostColorBuffer(param.hostColorBufferId,
            // myOutputWidth,
            //                             myOutputHeight, pDecodedFrame);
            elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime);
            H264_DPRINT("get image takes %lld ms\n\n", elapsed.count());
        } else {
            //memcpy(dst, pDecodedFrame, myOutputHeight * myOutputWidth * 3 / 2);
        }
    } else {
        memcpy(dst, pDecodedFrame, myOutputHeight * myOutputWidth * 3 / 2);
    }
    mSavedFrames.pop_front();
    mSavedPts.pop_front();
    mSavedW.pop_front();
    mSavedH.pop_front();
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
     elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - startTime);
     H264_DPRINT("get image takes %lld ms\n\n", elapsed.count());
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
    int nDecodeSurface = 8;  // pVideoFormat->min_num_decode_surfaces;

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
    g_mSurfaceHeight = videoDecodeCreateInfo.ulTargetHeight;

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
    auto startTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::milliseconds::zero();
    NVDEC_API_CALL(cuvidDecodePicture(mCudaDecoder, pPicParams));
    H264_DPRINT("successfully called.");
     elapsed = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - startTime);
     H264_DPRINT(" takes %lld ms", elapsed.count());
    return 1;
}

extern "C" {

void cuda_copy_decoded_frame(void* src_frame, uint32_t dest_texture_handle, int mode, int width, int height) {
//    NVDEC_API_CALL(cuCtxPushCurrent(g_mCudaContext));
//    NVDEC_API_CALL(cuStreamSynchronize(0));
    if (g_mGLCudaContext == nullptr) {
        CUdevice cudaDevice = 0;
        cuDeviceGet(&cudaDevice, 0);
        NVDEC_API_CALL(cuGLCtxCreate(&g_mGLCudaContext, CU_CTX_BLOCKING_SYNC, cudaDevice));
        H264_DPRINT("create gl cuda context successfully");
    }

//   NVDEC_API_CALL(cuCtxPushCurrent(g_mGLCudaContext));

    const unsigned int GL_TEXTURE_2D = 0x0DE1;
    const unsigned int cudaGraphicsMapFlagsNone = 0x0;
    CUgraphicsResource CudaRes {0};
    H264_DPRINT("cuda copy decoded frame testure %d", (int)dest_texture_handle);
    NVDEC_API_CALL(cuGraphicsGLRegisterImage(&CudaRes, dest_texture_handle, GL_TEXTURE_2D, 0x0));
    CUarray texture_ptr;
    NVDEC_API_CALL(cuGraphicsMapResources(1, &CudaRes, 0));
    NVDEC_API_CALL(cuGraphicsSubResourceGetMappedArray(&texture_ptr, CudaRes, 0, 0));

    CUdeviceptr dpSrcFrame = g_dpSrcFrame;
    CUDA_MEMCPY2D m = {0};
    m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
    m.srcDevice = dpSrcFrame;
    m.srcPitch = g_nSrcPitch;
    m.dstMemoryType = CU_MEMORYTYPE_ARRAY;
    //m.dstPtr = texture_ptr;
    m.dstArray = texture_ptr;
    m.dstPitch = width * 1;
    m.WidthInBytes = width * 1;
    m.Height = height;
    H264_DPRINT("dstPitch %d, WidthInBytes %d Height %d", (int)m.dstPitch, (int)m.WidthInBytes, (int)m.Height);
 
    if (mode == 1) { // copy Y data
        NVDEC_API_CALL(cuMemcpy2D(&m));
    } else if (mode == 2) { //copy UV data
        m.srcDevice = (CUdeviceptr)((uint8_t*)dpSrcFrame + m.srcPitch * g_mSurfaceHeight);
        m.Height = height/2;
        NVDEC_API_CALL(cuMemcpy2D(&m));
    }
    NVDEC_API_CALL(cuGraphicsUnmapResources(1, &CudaRes, 0));
//    NVDEC_API_CALL(cuStreamSynchronize(0));
//    NVDEC_API_CALL(cuCtxPopCurrent(NULL));
 //   NVDEC_API_CALL(cuCtxPopCurrent(NULL));
}

void cuda_nv12_updater(uint32_t Ytex, uint32_t UVtex) {
	H264_DPRINT("copyiong Ytex %d", Ytex);
	H264_DPRINT("copyiong UVtex %d", UVtex);
    cuda_copy_decoded_frame(nullptr, UVtex, 2, g_width, g_height);
    cuda_copy_decoded_frame(nullptr, Ytex, 1, g_width, g_height);
}
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

    g_dpSrcFrame = dpSrcFrame;
    g_nSrcPitch = nSrcPitch;
    g_width = mOutputWidth;
    g_height = mOutputHeight;
    g_mCudaContext = mCudaContext;

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
    H264_DPRINT("dstDevice %p, dstPitch %d, WidthInBytes %d Height %d pitch %d luma h %d and chrom h %d",
                m.dstHost, (int)m.dstPitch, (int)m.WidthInBytes, (int)m.Height, (int)nSrcPitch, (int)mLumaHeight,
                (int)mChromaHeight);

    //NVDEC_API_CALL(cuMemcpy2DAsync(&m, 0));

    m.srcDevice =
            (CUdeviceptr)((uint8_t*)dpSrcFrame + m.srcPitch * mSurfaceHeight);
    m.dstDevice =
            (CUdeviceptr)(m.dstHost = pDecodedFrame + m.dstPitch * mLumaHeight);
    m.Height = mChromaHeight;
    //NVDEC_API_CALL(cuMemcpy2DAsync(&m, 0));

    NVDEC_API_CALL(cuStreamSynchronize(0));

    auto startTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::milliseconds::zero();
    if (mParser.version() == 200) {
        if (g_hostColorBufferId >= 0 && g_gpuCopy) {
            // mRenderer.renderToHostColorBufferWithCallback(g_hostColorBufferId,
            // mOutputWidth,
            //                              mOutputHeight, nullptr,
            //                              cuda_copy_decoded_frame);
            MediaHostRenderer::TextureFrame texFrame =
                    mRenderer.getTextureFrame(g_width, g_height);
            mRenderer.saveFrameToTextures(texFrame, cuda_nv12_updater);
            if (!mIsLoadingFromSnapshot) {
                std::lock_guard<std::mutex> g(mFrameLock);
                mSavedTexFrames.push_back(texFrame);
            }
        }
    }
     elapsed = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now() - startTime);
     H264_DPRINT(" rendering takes %lld ms", elapsed.count());
    
    NVDEC_API_CALL(cuCtxPopCurrent(NULL));
    NVDEC_API_CALL(cuvidUnmapVideoFrame(mCudaDecoder, dpSrcFrame));
    if (!mIsLoadingFromSnapshot) {
        std::lock_guard<std::mutex> g(mFrameLock);
        mSavedFrames.push_back(myFrame);
        mSavedPts.push_back(myOutputPts);
        mSavedW.push_back(mOutputWidth);
        mSavedH.push_back(mOutputHeight);
        H264_DPRINT("decoded pts %lu", myOutputPts);
    }
    mImageReady = true;
    H264_DPRINT("successfully called.tid %ld saved frame %d", android_get_thread_id(),
            (int)mSavedPts.size());
    return 1;
}

void MediaH264DecoderCuvid::oneShotDecode(std::vector<uint8_t>& data,
                                          uint64_t pts) {
    H264_DPRINT("decoding pts %lld", (long long)pts);
    decodeFrameInternal(nullptr, nullptr, data.data(), data.size(), pts);
}

void MediaH264DecoderCuvid::save(base::Stream* stream) const {
    stream->putBe32(mParser.version());
    stream->putBe32(mWidth);
    stream->putBe32(mHeight);
    stream->putBe32(mOutputWidth);
    stream->putBe32(mOutputHeight);
    stream->putBe32((int)mOutPixFmt);

    const int hasContext = mCudaContext == nullptr ? 0 : 1;
    stream->putBe32(hasContext);

    mSnapshotState.savedFrames.clear();
    mSnapshotState.savedDecodedFrame.data.clear();
    for (size_t i = 0; i < mSavedFrames.size(); ++i) {
        const std::vector<uint8_t>& myFrame = mSavedFrames.front();
        int myOutputWidth = mSavedW.front();
        int myOutputHeight = mSavedH.front();
        int myOutputPts = mSavedPts.front();
        mSnapshotState.saveDecodedFrame(
                myFrame, myOutputWidth, myOutputHeight,
                ColorAspects{mColorPrimaries, mColorRange, mColorTransfer,
                             mColorSpace},
                myOutputPts);
        mSavedFrames.pop_front();
        mSavedW.pop_front();
        mSavedH.pop_front();
        mSavedPts.pop_front();
    }
    H264_DPRINT("saving packets now %d",
                (int)(mSnapshotState.savedPackets.size()));
    mSnapshotState.save(stream);
}

bool MediaH264DecoderCuvid::load(base::Stream* stream) {
    mIsLoadingFromSnapshot = true;
    uint32_t version = stream->getBe32();
    mParser = H264PingInfoParser{version};

    mWidth = stream->getBe32();
    mHeight = stream->getBe32();
    mOutputWidth = stream->getBe32();
    mOutputHeight = stream->getBe32();
    mOutPixFmt = (PixelFormat)stream->getBe32();

    const int hasContext = stream->getBe32();
    if (hasContext) {
        initH264ContextInternal(mWidth, mHeight, mWidth, mHeight, mOutPixFmt);
    }

    mSnapshotState.load(stream);

    H264_DPRINT("loaded packets %d, now restore decoder",
                (int)(mSnapshotState.savedPackets.size()));
    if (hasContext && mSnapshotState.sps.size() > 0) {
        oneShotDecode(mSnapshotState.sps, 0);
        if (mSnapshotState.pps.size() > 0) {
            oneShotDecode(mSnapshotState.pps, 0);
            if (mSnapshotState.savedPackets.size() > 0) {
                for (int i = 0; i < mSnapshotState.savedPackets.size(); ++i) {
                    PacketInfo& pkt = mSnapshotState.savedPackets[i];
                    oneShotDecode(pkt.data, pkt.pts);
                }
            }
        }
    }

    mImageReady = false;
    for (size_t i = 0; i < mSnapshotState.savedFrames.size(); ++i) {
        auto& frame = mSnapshotState.savedFrames[i];
        mOutBufferSize = frame.data.size();
        mOutputWidth = frame.width;
        mOutputHeight = frame.height;
        mColorPrimaries = frame.color.primaries;
        mColorRange = frame.color.range;
        mColorTransfer = frame.color.transfer;
        mColorSpace = frame.color.space;
        mOutputPts = frame.pts;
        mSavedFrames.push_back(frame.data);
        mSavedW.push_back(mOutputWidth);
        mSavedH.push_back(mOutputHeight);
        mSavedPts.push_back(mOutputPts);
        mImageReady = true;
    }
    mIsLoadingFromSnapshot = false;
    return true;
}

bool MediaH264DecoderCuvid::s_isCudaInitialized = false;
// static

}  // namespace emulation
}  // namespace android
