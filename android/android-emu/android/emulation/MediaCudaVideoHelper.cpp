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

#include "android/emulation/MediaCudaVideoHelper.h"
#include "android/emulation/MediaCudaDriverHelper.h"
#include "android/emulation/MediaCudaUtils.h"
#include "android/emulation/YuvConverter.h"
#include "android/utils/debug.h"

extern "C" {
#define INIT_CUDA_GL 1
#include "android/emulation/dynlink_cuda.h"
#include "android/emulation/dynlink_cudaGL.h"
#include "android/emulation/dynlink_nvcuvid.h"
}
#define MEDIA_CUDA_DEBUG 0

#if MEDIA_CUDA_DEBUG
#define CUDA_DPRINT(fmt, ...)                                             \
    fprintf(stderr, "media-cuda-video-helper: %s:%d " fmt "\n", __func__, \
            __LINE__, ##__VA_ARGS__);
#else
#define CUDA_DPRINT(fmt, ...)
#endif

#define NVDEC_API_CALL(cuvidAPI)                                     \
    do {                                                             \
        CUresult errorCode = cuvidAPI;                               \
        if (errorCode != CUDA_SUCCESS) {                             \
            CUDA_DPRINT("%s failed with error code %d\n", #cuvidAPI, \
                        (int)errorCode);                             \
        }                                                            \
    } while (0)

namespace android {
namespace emulation {

bool MediaCudaVideoHelper::s_isCudaDecoderGood = true;

using TextureFrame = MediaTexturePool::TextureFrame;
using FrameInfo = MediaSnapshotState::FrameInfo;
using ColorAspects = MediaSnapshotState::ColorAspects;

MediaCudaVideoHelper::MediaCudaVideoHelper(OutputTreatmentMode oMode,
                                           FrameStorageMode fMode,
                                           cudaVideoCodec cudaVideoCodecType)
    : mUseGpuTexture(fMode == FrameStorageMode::USE_GPU_TEXTURE),
      mCudaVideoCodecType(cudaVideoCodecType) {
    mIgnoreDecoderOutput = (oMode == OutputTreatmentMode::IGNORE_RESULT);
}

MediaCudaVideoHelper::~MediaCudaVideoHelper() {
    deInit();
}

void MediaCudaVideoHelper::deInit() {
    CUDA_DPRINT("deInit calling");

    mSavedDecodedFrames.clear();
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
            CUDA_DPRINT("Failed to destroy cuda context; error code %d",
                        (int)myres);
        }
        mCudaContext = nullptr;
    }
}

bool MediaCudaVideoHelper::init() {
    if (!s_isCudaDecoderGood) {
        CUDA_DPRINT(
                "Already verified: cuda decoder does not work on this host");
        return false;
    }
    if (!MediaCudaDriverHelper::initCudaDrivers()) {
        CUDA_DPRINT("Failed to initCudaDrivers");
        mIsGood = false;
        mErrorCode = 1;
        s_isCudaDecoderGood = false;
        return false;
    }

    if (mCudaContext != nullptr) {
        deInit();
    }

    // cudat stuff
    const int gpuIndex = 0;
    const int cudaFlags = 0;
    CUdevice cudaDevice = 0;
    CUresult myres = cuDeviceGet(&cudaDevice, gpuIndex);
    if (myres != CUDA_SUCCESS) {
        mIsGood = false;
        mErrorCode = 2;
        s_isCudaDecoderGood = false;
        CUDA_DPRINT("Failed to get cuda device, error code %d", (int)myres);
        return false;
    }

    char buf[1024];
    myres = cuDeviceGetName(buf, sizeof(buf), cudaDevice);
    if (myres != CUDA_SUCCESS) {
        mIsGood = false;
        mErrorCode = 3;
        s_isCudaDecoderGood = false;
        CUDA_DPRINT("Failed to get gpu device name, error code %d", (int)myres);
        return false;
    }

    CUDA_DPRINT("using gpu device %s", buf);

    myres = cuCtxCreate(&mCudaContext, cudaFlags, cudaDevice);
    if (myres != CUDA_SUCCESS) {
        mIsGood = false;
        s_isCudaDecoderGood = false;
        CUDA_DPRINT("Failed to create cuda context, error code %d", (int)myres);
        return false;
    }

    NVDEC_API_CALL(cuvidCtxLockCreate(&mCtxLock, mCudaContext));

    CUVIDPARSERPARAMS videoParserParameters = {};
    // videoParserParameters.CodecType = (mType == MediaCodecType::VP8Codec) ?
    // cudaVideoCodec_VP8 : cudaVideoCodec_VP9;
    videoParserParameters.CodecType = mCudaVideoCodecType;

    videoParserParameters.ulMaxNumDecodeSurfaces = 1;
    videoParserParameters.ulMaxDisplayDelay = 1;
    videoParserParameters.pUserData = this;
    videoParserParameters.pfnSequenceCallback = HandleVideoSequenceProc;
    videoParserParameters.pfnDecodePicture = HandlePictureDecodeProc;
    videoParserParameters.pfnDisplayPicture = HandlePictureDisplayProc;
    NVDEC_API_CALL(
            cuvidCreateVideoParser(&mCudaParser, &videoParserParameters));

    CUDA_DPRINT("Successfully created cuda context %p", mCudaContext);
    dprint("successfully created cuda video decoder for %s, with gpu texture "
           "mode %s",
           mCudaVideoCodecType == cudaVideoCodec_H264
                   ? "H264"
                   : (mCudaVideoCodecType == cudaVideoCodec_VP8 ? "VP8"
                                                                : "VP9"),
           mUseGpuTexture ? "on" : "off");

    return true;
}

void MediaCudaVideoHelper::decode(const uint8_t* frame,
                                  size_t szBytes,
                                  uint64_t inputPts) {
    CUDA_DPRINT("%s(frame=%p, sz=%zu)", __func__, frame, szBytes);

    CUVIDSOURCEDATAPACKET packet = {0};
    packet.payload = frame;
    packet.payload_size = szBytes;
    packet.flags = CUVID_PKT_TIMESTAMP;
    packet.timestamp = inputPts;
    if (!frame || szBytes == 0) {
        packet.flags |= CUVID_PKT_ENDOFSTREAM;
    } else {
        ++mNumInputFrame;
    }
    NVDEC_API_CALL(cuvidParseVideoData(mCudaParser, &packet));
}

void MediaCudaVideoHelper::flush() {
    CUDA_DPRINT("started flushing");
    CUVIDSOURCEDATAPACKET packet = {0};
    packet.payload = NULL;
    packet.payload_size = 0;
    packet.flags |= CUVID_PKT_ENDOFSTREAM;
    NVDEC_API_CALL(cuvidParseVideoData(mCudaParser, &packet));
    CUDA_DPRINT("done one flushing");
}

int MediaCudaVideoHelper::HandleVideoSequence(CUVIDEOFORMAT* pVideoFormat) {
    int nDecodeSurface = 8;  // need 8 for 4K video

    CUVIDDECODECAPS decodecaps;
    memset(&decodecaps, 0, sizeof(decodecaps));

    decodecaps.eCodecType = pVideoFormat->codec;
    decodecaps.eChromaFormat = pVideoFormat->chroma_format;
    decodecaps.nBitDepthMinus8 = pVideoFormat->bit_depth_luma_minus8;

    NVDEC_API_CALL(cuCtxPushCurrent(mCudaContext));
    NVDEC_API_CALL(cuvidGetDecoderCaps(&decodecaps));
    NVDEC_API_CALL(cuCtxPopCurrent(NULL));

    if (!decodecaps.bIsSupported) {
        mIsGood = false;
        mErrorCode = 4;
        CUDA_DPRINT("Codec not supported on this GPU.");
        return nDecodeSurface;
    }

    if ((pVideoFormat->coded_width > decodecaps.nMaxWidth) ||
        (pVideoFormat->coded_height > decodecaps.nMaxHeight)) {
        CUDA_DPRINT("Resolution not supported on this GPU");
        mIsGood = false;
        mErrorCode = 5;
        return nDecodeSurface;
    }

    if ((pVideoFormat->coded_width >> 4) * (pVideoFormat->coded_height >> 4) >
        decodecaps.nMaxMBCount) {
        CUDA_DPRINT("MBCount not supported on this GPU");
        mIsGood = false;
        mErrorCode = 6;
        return nDecodeSurface;
    }

    mLumaWidth =
            pVideoFormat->display_area.right - pVideoFormat->display_area.left;
    mLumaHeight =
            pVideoFormat->display_area.bottom - pVideoFormat->display_area.top;
    mChromaHeight = mLumaHeight * 0.5;  // NV12
    mBPP = pVideoFormat->bit_depth_luma_minus8 > 0 ? 2 : 1;

    if (mCudaVideoCodecType == cudaVideoCodec_H264) {
        if (pVideoFormat->video_signal_description.video_full_range_flag)
            mColorRange = 2;
        else
            mColorRange = 0;

        mColorPrimaries =
                pVideoFormat->video_signal_description.color_primaries;
        mColorTransfer =
                pVideoFormat->video_signal_description.transfer_characteristics;
        mColorSpace =
                pVideoFormat->video_signal_description.matrix_coefficients;
    }

    CUVIDDECODECREATEINFO videoDecodeCreateInfo = {0};
    videoDecodeCreateInfo.CodecType = pVideoFormat->codec;
    videoDecodeCreateInfo.ChromaFormat = pVideoFormat->chroma_format;
    videoDecodeCreateInfo.OutputFormat = cudaVideoSurfaceFormat_NV12;
    CUDA_DPRINT("output format is %d", videoDecodeCreateInfo.OutputFormat);
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
        CUDA_DPRINT("old width %d old height %d", mOutputWidth, mOutputHeight);
        mOutputWidth = mLumaWidth;
        mOutputHeight = mLumaHeight;
        CUDA_DPRINT("new width %d new height %d", mOutputWidth, mOutputHeight);
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
        CUDA_DPRINT("free memory %g M, total %g M", free / 1048576.0,
                    total / 1048576.0);
    }
    NVDEC_API_CALL(cuCtxPopCurrent(NULL));
    NVDEC_API_CALL(cuCtxPushCurrent(mCudaContext));
    NVDEC_API_CALL(cuvidCreateDecoder(&mCudaDecoder, &videoDecodeCreateInfo));
    NVDEC_API_CALL(cuCtxPopCurrent(NULL));
    CUDA_DPRINT("successfully called. decoder %p", mCudaDecoder);
    return nDecodeSurface;
}

int MediaCudaVideoHelper::HandlePictureDecode(CUVIDPICPARAMS* pPicParams) {
    NVDEC_API_CALL(cuvidDecodePicture(mCudaDecoder, pPicParams));
    CUDA_DPRINT("successfully called.");
    return 1;
}

int MediaCudaVideoHelper::HandlePictureDisplay(CUVIDPARSERDISPINFO* pDispInfo) {
    if (mIgnoreDecoderOutput) {
        return 1;
    }
    constexpr int MAX_NUM_INPUT_WITHOUT_OUTPUT = 16;
    if (mNumOutputFrame == 0 && mNumInputFrame > MAX_NUM_INPUT_WITHOUT_OUTPUT) {
        // after more than 16 inputs, there is still no output,
        // probably corrupted stream, ignore everything from now on
        dprint("WARNING: %d frames decoded witout any output, possibly bad "
               "input stream. Ignore output frames (they might be corrupted) "
               "from now on.",
               MAX_NUM_INPUT_WITHOUT_OUTPUT);
        return 0;
    }

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
    CUresult errorCode = cuvidMapVideoFrame(mCudaDecoder, pDispInfo->picture_index,
                                      &dpSrcFrame, &nSrcPitch,
                                      &videoProcessingParameters);
    if (errorCode != CUDA_SUCCESS) {
        CUDA_DPRINT("failed to call cuvidMapVideoFrame with error code %d\n", (int)errorCode);
        return 0;
    }

    NVDEC_API_CALL(cuCtxPushCurrent(mCudaContext));
    unsigned int newOutBufferSize = mOutputWidth * mOutputHeight * 3 / 2;
    std::vector<uint8_t> myFrame;
    TextureFrame texFrame;
    if (mUseGpuTexture && mTexturePool != nullptr) {
        media_cuda_utils_copy_context my_copy_context{
                .src_frame = dpSrcFrame,
                .src_pitch = nSrcPitch,
                .src_surface_height = mSurfaceHeight,
                .dest_width = mOutputWidth,
                .dest_height = mOutputHeight,
        };
        texFrame = mTexturePool->getTextureFrame(mOutputWidth, mOutputHeight);
        mTexturePool->saveDecodedFrameToTexture(
                texFrame, &my_copy_context,
                (void*)media_cuda_utils_nv12_updater);
    } else {
        myFrame.resize(newOutBufferSize);
        uint8_t* pDecodedFrame = &(myFrame[0]);

        CUDA_MEMCPY2D m = {0};
        m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
        m.srcDevice = dpSrcFrame;
        m.srcPitch = nSrcPitch;
        m.dstMemoryType = CU_MEMORYTYPE_HOST;
        m.dstDevice = (CUdeviceptr)(m.dstHost = pDecodedFrame);
        m.dstPitch = mOutputWidth * mBPP;
        m.WidthInBytes = mOutputWidth * mBPP;
        m.Height = mLumaHeight;
        CUDA_DPRINT("dstDevice %p, dstPitch %d, WidthInBytes %d Height %d",
                    m.dstHost, (int)m.dstPitch, (int)m.WidthInBytes,
                    (int)m.Height);

        NVDEC_API_CALL(cuMemcpy2DAsync(&m, 0));

        m.srcDevice = (CUdeviceptr)((uint8_t*)dpSrcFrame +
                                    m.srcPitch * mSurfaceHeight);
        m.dstDevice = (CUdeviceptr)(m.dstHost = pDecodedFrame +
                                                m.dstPitch * mLumaHeight);
        m.Height = mChromaHeight;
        NVDEC_API_CALL(cuMemcpy2DAsync(&m, 0));
        YuvConverter<uint8_t> convert8(mOutputWidth, mOutputHeight);
        convert8.UVInterleavedToPlanar(pDecodedFrame);
    }

    NVDEC_API_CALL(cuStreamSynchronize(0));
    NVDEC_API_CALL(cuCtxPopCurrent(NULL));

    NVDEC_API_CALL(cuvidUnmapVideoFrame(mCudaDecoder, dpSrcFrame));
    {
        std::lock_guard<std::mutex> g(mFrameLock);

        mSavedDecodedFrames.push_back(MediaSnapshotState::FrameInfo{
                std::move(myFrame),
                std::vector<uint32_t>{texFrame.Ytex, texFrame.UVtex},
                (int)mOutputWidth, (int)mOutputHeight, myOutputPts,
                ColorAspects{mColorPrimaries, mColorRange, mColorTransfer,
                             mColorSpace}});
    }
    ++mNumOutputFrame;
    CUDA_DPRINT("successfully called.");
    return 1;
}

}  // namespace emulation
}  // namespace android
