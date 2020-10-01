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
#include "android/main-emugl.h"
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
#define INIT_CUDA_GL 1
#include "android/emulation/dynlink_cuda.h"
#include "android/emulation/dynlink_cudaGL.h"
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
using TextureFrame = MediaHostRenderer::TextureFrame;

MediaH264DecoderCuvid::MediaH264DecoderCuvid(uint64_t id,
                                             H264PingInfoParser parser)
    : mId(id), mParser(parser) {
    auto useGpuTextureEnv = android::base::System::getEnvironmentVariable(
            "ANDROID_EMU_CODEC_USE_GPU_TEXTURE");
    if (useGpuTextureEnv != "") {
        if (mParser.version() == 200) {
            if (emuglConfig_get_current_renderer() == SELECTED_RENDERER_HOST) {
                mUseGpuTexture = true;
            } else {
                H264_DPRINT(
                        "cannot use gpu texture to save decoded frame in "
                        "non-host gpu mode");
                if (emuglConfig_get_current_renderer() ==
                    SELECTED_RENDERER_SWIFTSHADER_INDIRECT) {
                    H264_DPRINT("your gpu mode is: swiftshader_indirect");
                }
            }
        }
    }
};

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

    for (auto texFrame : mSavedTexFrames) {
            mRenderer.putTextureFrame(texFrame);
    }
    mRenderer.cleanUpTextures();
    mSavedTexFrames.clear();
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

    const bool enableSnapshot = true;
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
                        szBytes, inputPts);
}

void MediaH264DecoderCuvid::decodeFrameInternal(uint64_t* pRetSzBytes,
                                                int32_t* pRetErr,
                                                const uint8_t* frame,
                                                size_t szBytes,
                                                uint64_t inputPts) {
    mIsInFlush = false;
    H264_DPRINT("%s(frame=%p, sz=%zu)", __func__, frame, szBytes);
    Err h264Err = Err::NoErr;

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
    uint64_t* retPts = param.pRetPts;
    uint32_t* retColorPrimaries = param.pRetColorPrimaries;
    uint32_t* retColorRange = param.pRetColorRange;
    uint32_t* retColorTransfer = param.pRetColorTransfer;
    uint32_t* retColorSpace = param.pRetColorSpace;

    static int numbers = 0;
    H264_DPRINT("calling getImage %d colorbuffer %d", numbers++,
                (int)param.hostColorBufferId);
    doFlush();
    uint8_t* dst = param.pDecodedFrame;
    int myOutputWidth = mOutputWidth;
    int myOutputHeight = mOutputHeight;
    std::vector<uint8_t> decodedFrame;
    TextureFrame decodedTexFrame;
    {
        std::lock_guard<std::mutex> g(mFrameLock);
        mImageReady = !mSavedFrames.empty();
        if (!mImageReady) {
            H264_DPRINT("%s: no new frame yet", __func__);
            *retErr = static_cast<int>(Err::NoDecodedFrame);
            return;
        }

        std::vector<uint8_t>& myFrame = mSavedFrames.front();
        std::swap(decodedFrame, myFrame);
        decodedTexFrame = mSavedTexFrames.front();
        mOutputPts = mSavedPts.front();

        myOutputWidth = mSavedW.front();
        myOutputHeight = mSavedH.front();
        *retWidth = myOutputWidth;
        *retHeight = myOutputHeight;

        mSavedFrames.pop_front();
        mSavedTexFrames.pop_front();
        mSavedPts.pop_front();
        mSavedW.pop_front();
        mSavedH.pop_front();
    }

    bool needToCopyToGuest = true;

    if (mUseGpuTexture) {
        needToCopyToGuest = false;
    } else {
        YuvConverter<uint8_t> convert8(myOutputWidth, myOutputHeight);
        convert8.UVInterleavedToPlanar(decodedFrame.data());
    }

    if (mParser.version() == 200) {
        if (param.hostColorBufferId >= 0) {
            needToCopyToGuest = false;
            if (mUseGpuTexture) {
                mRenderer.renderToHostColorBufferWithTextures(
                        param.hostColorBufferId, myOutputWidth, myOutputHeight,
                        decodedTexFrame);
            } else {
                mRenderer.renderToHostColorBuffer(param.hostColorBufferId,
                                                  myOutputWidth, myOutputHeight,
                                                  decodedFrame.data());
            }
        } else {
            if (mUseGpuTexture) {
                // no colorbuffer to send the textures to, just recycle
                // them back to Renderer
                mRenderer.putTextureFrame(decodedTexFrame);
            }
        }
    }

    if (needToCopyToGuest) {
        memcpy(dst, decodedFrame.data(),
               myOutputHeight * myOutputWidth * 3 / 2);
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

extern "C" {

#define MEDIA_H264_COPY_Y_TEXTURE 1
#define MEDIA_H264_COPY_UV_TEXTURE 2

struct h264_cuvid_copy_context {
    CUdeviceptr src_frame;
    unsigned int src_pitch;

    // this usually >= dest_height due to padding, e.g.
    // src_surface_height: 1088, dest_height: 1080
    // so, when copying UV data, the src has to start at
    // offset = src_pitch * src_surface_height
    unsigned int src_surface_height;

    unsigned int dest_width;
    unsigned int dest_height;
};

void cuda_copy_decoded_frame(void* privData,
                             int mode,
                             uint32_t dest_texture_handle) {
    h264_cuvid_copy_context* copy_context =
            static_cast<h264_cuvid_copy_context*>(privData);

    const unsigned int GL_TEXTURE_2D = 0x0DE1;
    const unsigned int cudaGraphicsMapFlagsNone = 0x0;
    CUgraphicsResource CudaRes{0};
    H264_DPRINT("cuda copy decoded frame testure %d", (int)dest_texture_handle);
    NVDEC_API_CALL(cuGraphicsGLRegisterImage(&CudaRes, dest_texture_handle,
                                             GL_TEXTURE_2D, 0x0));
    CUarray texture_ptr;
    NVDEC_API_CALL(cuGraphicsMapResources(1, &CudaRes, 0));
    NVDEC_API_CALL(
            cuGraphicsSubResourceGetMappedArray(&texture_ptr, CudaRes, 0, 0));
    CUdeviceptr dpSrcFrame = copy_context->src_frame;
    CUDA_MEMCPY2D m = {0};
    m.srcMemoryType = CU_MEMORYTYPE_DEVICE;
    m.srcDevice = dpSrcFrame;
    m.srcPitch = copy_context->src_pitch;
    m.dstMemoryType = CU_MEMORYTYPE_ARRAY;
    m.dstArray = texture_ptr;
    m.dstPitch = copy_context->dest_width * 1;
    m.WidthInBytes = copy_context->dest_width * 1;
    m.Height = copy_context->dest_height;
    H264_DPRINT("dstPitch %d, WidthInBytes %d Height %d surface-height %d",
                (int)m.dstPitch, (int)m.WidthInBytes, (int)m.Height,
                (int)copy_context->src_surface_height);

    if (mode == MEDIA_H264_COPY_Y_TEXTURE) {  // copy Y data
        NVDEC_API_CALL(cuMemcpy2D(&m));
    } else if (mode == MEDIA_H264_COPY_UV_TEXTURE) {  // copy UV data
        m.srcDevice =
                (CUdeviceptr)((uint8_t*)dpSrcFrame +
                              m.srcPitch * copy_context->src_surface_height);
        m.Height = m.Height / 2;
        NVDEC_API_CALL(cuMemcpy2D(&m));
    }
    NVDEC_API_CALL(cuGraphicsUnmapResources(1, &CudaRes, 0));
    NVDEC_API_CALL(cuGraphicsUnregisterResource(CudaRes));
}

void cuda_nv12_updater(void* privData, uint32_t type, uint32_t* textures) {
    constexpr uint32_t kFRAMEWORK_FORMAT_NV12 = 3;
    if (type != kFRAMEWORK_FORMAT_NV12) {
        return;
    }
    H264_DPRINT("copyiong Ytex %d", textures[0]);
    H264_DPRINT("copyiong UVtex %d", textures[1]);
    cuda_copy_decoded_frame(privData, MEDIA_H264_COPY_Y_TEXTURE, textures[0]);
    cuda_copy_decoded_frame(privData, MEDIA_H264_COPY_UV_TEXTURE, textures[1]);
}

}  // end extern C

int MediaH264DecoderCuvid::HandlePictureDisplay(
        CUVIDPARSERDISPINFO* pDispInfo) {
    if (mIsLoadingFromSnapshot) {
        return 1;
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
    NVDEC_API_CALL(cuvidMapVideoFrame(mCudaDecoder, pDispInfo->picture_index,
                                      &dpSrcFrame, &nSrcPitch,
                                      &videoProcessingParameters));

    NVDEC_API_CALL(cuCtxPushCurrent(mCudaContext));
    unsigned int newOutBufferSize = mOutputWidth * mOutputHeight * 3 / 2;
    std::vector<uint8_t> myFrame;
    TextureFrame texFrame;
    if (mUseGpuTexture) {
            h264_cuvid_copy_context my_copy_context{
                    .src_frame = dpSrcFrame,
                    .src_pitch = nSrcPitch,
                    .src_surface_height = mSurfaceHeight,
                    .dest_width = mOutputWidth,
                    .dest_height = mOutputHeight,
            };
            texFrame = mRenderer.getTextureFrame(mOutputWidth, mOutputHeight);
            mRenderer.saveDecodedFrameToTexture(texFrame, &my_copy_context,
                                                (void*)cuda_nv12_updater);
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
        H264_DPRINT("dstDevice %p, dstPitch %d, WidthInBytes %d Height %d",
                    m.dstHost, (int)m.dstPitch, (int)m.WidthInBytes,
                    (int)m.Height);

        NVDEC_API_CALL(cuMemcpy2DAsync(&m, 0));

        m.srcDevice = (CUdeviceptr)((uint8_t*)dpSrcFrame +
                                    m.srcPitch * mSurfaceHeight);
        m.dstDevice = (CUdeviceptr)(m.dstHost = pDecodedFrame +
                                                m.dstPitch * mLumaHeight);
        m.Height = mChromaHeight;
        NVDEC_API_CALL(cuMemcpy2DAsync(&m, 0));
    }

    NVDEC_API_CALL(cuStreamSynchronize(0));
    NVDEC_API_CALL(cuCtxPopCurrent(NULL));

    NVDEC_API_CALL(cuvidUnmapVideoFrame(mCudaDecoder, dpSrcFrame));
    if (!mIsLoadingFromSnapshot) {
        std::lock_guard<std::mutex> g(mFrameLock);
        mSavedFrames.push_back(myFrame);
        mSavedTexFrames.push_back(texFrame);
        mSavedPts.push_back(myOutputPts);
        mSavedW.push_back(mOutputWidth);
        mSavedH.push_back(mOutputHeight);
    }
    mImageReady = true;
    H264_DPRINT("successfully called.");
    return 1;
}

void MediaH264DecoderCuvid::oneShotDecode(std::vector<uint8_t>& data,
                                          uint64_t pts) {
    H264_DPRINT("decoding pts %lld", (long long)pts);
    decodeFrameInternal(nullptr, nullptr, data.data(), data.size(), pts);
}

void MediaH264DecoderCuvid::save(base::Stream* stream) const {
    stream->putBe32(mParser.version());
    const int useGpuTexture = mUseGpuTexture ? 1 : 0;
    stream->putBe32(useGpuTexture);

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
        mSavedTexFrames.pop_front();
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
    const int useGpuTexture = stream->getBe32();
    mUseGpuTexture = useGpuTexture ? true : false;

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
        TextureFrame texFrame =
                mRenderer.getTextureFrame(mOutputWidth, mOutputHeight);
        mSavedTexFrames.push_back(texFrame);
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
