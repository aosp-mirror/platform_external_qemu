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

#include "android/emulation/MediaVpxDecoderCuvid.h"
#include "android/emulation/YuvConverter.h"
#include "android/main-emugl.h"
// MediaVpxDecoderCuvid.h
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
#define MEDIA_Vpx_DEBUG 1

#if MEDIA_Vpx_DEBUG
#define VPX_DPRINT(fmt, ...)                                              \
    fprintf(stderr, "vpx-decoder-using-cuvid: %s:%d " fmt "\n", __func__, \
            __LINE__, ##__VA_ARGS__);
#else
#define VPX_DPRINT(fmt, ...)
#endif

#define NVDEC_API_CALL(cuvidAPI)                                    \
    do {                                                            \
        CUresult errorCode = cuvidAPI;                              \
        if (errorCode != CUDA_SUCCESS) {                            \
            VPX_DPRINT("%s failed with error code %d\n", #cuvidAPI, \
                       (int)errorCode);                             \
        }                                                           \
    } while (0)

namespace android {
namespace emulation {

using InitContextParam = VpxPingInfoParser::InitContextParam;
using DecodeFrameParam = VpxPingInfoParser::DecodeFrameParam;
using GetImageParam = VpxPingInfoParser::GetImageParam;
using TextureFrame = MediaHostRenderer::TextureFrame;

MediaVpxDecoderCuvid::MediaVpxDecoderCuvid(VpxPingInfoParser parser,
                                           MediaCodecType type)
    : mParser(parser), mType(type) {
    auto useGpuTextureEnv = android::base::System::getEnvironmentVariable(
            "ANDROID_EMU_CODEC_USE_GPU_TEXTURE");
    if (useGpuTextureEnv != "") {
        if (mParser.version() == 200) {
            if (emuglConfig_get_current_renderer() == SELECTED_RENDERER_HOST) {
                mUseGpuTexture = true;
            } else {
                VPX_DPRINT(
                        "cannot use gpu texture to save decoded frame in "
                        "non-host gpu mode");
                if (emuglConfig_get_current_renderer() ==
                    SELECTED_RENDERER_SWIFTSHADER_INDIRECT) {
                    VPX_DPRINT("your gpu mode is: swiftshader_indirect");
                }
            }
        }
    }
};

MediaVpxDecoderCuvid::~MediaVpxDecoderCuvid() {
    destroyVpxContext(nullptr);
}

void MediaVpxDecoderCuvid::initVpxContext(void* ptr) {
    InitContextParam param{};
    mParser.parseInitContextParams(ptr, param);
    initVpxContextInternal();
}


void MediaVpxDecoderCuvid::decodeFrame(void* ptr) {
    DecodeFrameParam param{};
    mParser.parseDecodeFrameParams(ptr, param);

    const uint8_t* frame = param.p_data;
    size_t szBytes = param.size;
    // TODO: fix the guest to use pts directly
    uint64_t inputPts = param.user_priv;  // param.user_priv;

    decodeFrameInternal(frame, szBytes, inputPts);
}


void MediaVpxDecoderCuvid::getImage(void* ptr) {
    VPX_DPRINT("getImage %p", ptr);
    GetImageParam param{};
    mParser.parseGetImageParams(ptr, param);

    int* retErr = param.p_error;
    uint32_t* retWidth = param.p_d_w;
    uint32_t* retHeight = param.p_d_h;
    uint64_t * retPts = param.p_user_priv;

    // fixed the format
    constexpr int VPX_IMG_FMT_PLANAR = 0x100;
    constexpr int VPX_IMG_FMT_I420 = VPX_IMG_FMT_PLANAR | 2;
    *(param.p_fmt) = VPX_IMG_FMT_I420;

    static int numbers = 0;
    VPX_DPRINT("calling getImage %d colorbuffer %d", numbers++,
               (int)param.hostColorBufferId);
    doFlush();
    uint8_t* dst = param.p_dst;
    int myOutputWidth = mOutputWidth;
    int myOutputHeight = mOutputHeight;
    std::vector<uint8_t> decodedFrame;
    TextureFrame decodedTexFrame;
    {
        std::lock_guard<std::mutex> g(mFrameLock);
        mImageReady = !mSavedFrames.empty();
        if (!mImageReady) {
            VPX_DPRINT("%s: no new frame yet", __func__);
            *retErr = 1;
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
    *retErr = 0;
    *retPts = mOutputPts;
    VPX_DPRINT("Copying completed pts %lld", (long long)mOutputPts);
}

void MediaVpxDecoderCuvid::flush(void* ptr) {
    mIsInFlush = true;
    doFlush();
}

void MediaVpxDecoderCuvid::destroyVpxContext(void* ptr) {
    VPX_DPRINT("destroyVpxContext calling");

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
            VPX_DPRINT("Failed to destroy cuda context; error code %d",
                       (int)myres);
        }
        mCudaContext = nullptr;
    }
}

void MediaVpxDecoderCuvid::doFlush() {
    if (!mIsInFlush) {
        return;
    }
    VPX_DPRINT("started flushing");
    CUVIDSOURCEDATAPACKET packet = {0};
    packet.payload = NULL;
    packet.payload_size = 0;
    packet.flags |= CUVID_PKT_ENDOFSTREAM;
    NVDEC_API_CALL(cuvidParseVideoData(mCudaParser, &packet));
    VPX_DPRINT("done one flushing");
}

void MediaVpxDecoderCuvid::decodeFrameInternal(const uint8_t* frame,
                                               size_t szBytes,
                                               uint64_t inputPts) {
    mIsInFlush = false;
    VPX_DPRINT("%s(frame=%p, sz=%zu)", __func__, frame, szBytes);

    CUVIDSOURCEDATAPACKET packet = {0};
    packet.payload = frame;
    packet.payload_size = szBytes;
    packet.flags = CUVID_PKT_TIMESTAMP;
    packet.timestamp = inputPts;
    if (!frame || szBytes == 0) {
        packet.flags |= CUVID_PKT_ENDOFSTREAM;
    }
    NVDEC_API_CALL(cuvidParseVideoData(mCudaParser, &packet));
}

void MediaVpxDecoderCuvid::initVpxContextInternal() {
    if (!initCudaDrivers()) {
        VPX_DPRINT("Failed to initVpxContext because driver is not working");
        return;
    }

    if (mCudaContext != nullptr) {
        destroyVpxContext(nullptr);
    }

    // cudat stuff
    const int gpuIndex = 0;
    const int cudaFlags = 0;
    CUdevice cudaDevice = 0;
    CUresult myres = cuDeviceGet(&cudaDevice, gpuIndex);
    if (myres != CUDA_SUCCESS) {
        VPX_DPRINT("Failed to get cuda device, error code %d", (int)myres);
        return;
    }

    char buf[1024];
    myres = cuDeviceGetName(buf, sizeof(buf), cudaDevice);
    if (myres != CUDA_SUCCESS) {
        VPX_DPRINT("Failed to get gpu device name, error code %d", (int)myres);
        return;
    }

    VPX_DPRINT("using gpu device %s", buf);

    myres = cuCtxCreate(&mCudaContext, cudaFlags, cudaDevice);
    if (myres != CUDA_SUCCESS) {
        VPX_DPRINT("Failed to create cuda context, error code %d", (int)myres);
    }

    NVDEC_API_CALL(cuvidCtxLockCreate(&mCtxLock, mCudaContext));

    CUVIDPARSERPARAMS videoParserParameters = {};
    videoParserParameters.CodecType = (mType == MediaCodecType::VP8Codec)
                                              ? cudaVideoCodec_VP8
                                              : cudaVideoCodec_VP9;
    videoParserParameters.ulMaxNumDecodeSurfaces = 1;
    videoParserParameters.ulMaxDisplayDelay = 1;
    videoParserParameters.pUserData = this;
    videoParserParameters.pfnSequenceCallback = HandleVideoSequenceProc;
    videoParserParameters.pfnDecodePicture = HandlePictureDecodeProc;
    videoParserParameters.pfnDisplayPicture = HandlePictureDisplayProc;
    NVDEC_API_CALL(
            cuvidCreateVideoParser(&mCudaParser, &videoParserParameters));

    VPX_DPRINT("Successfully created cuda context %p", mCudaContext);
}

bool MediaVpxDecoderCuvid::initCudaDrivers() {
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
        VPX_DPRINT(
                "Failed to get number of GPU cards installed on host; error "
                "code %d",
                (int)myres);
        return false;
    }

    if (numGpuCards <= 0) {
        VPX_DPRINT("There are no nvidia GPU cards on this host.");
        return false;
    }

    // lukily, we get cuda initialized.
    s_isCudaInitialized = true;

    return true;
}

int MediaVpxDecoderCuvid::HandleVideoSequence(CUVIDEOFORMAT* pVideoFormat) {
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
        VPX_DPRINT("Codec not supported on this GPU.");
        return nDecodeSurface;
    }

    if ((pVideoFormat->coded_width > decodecaps.nMaxWidth) ||
        (pVideoFormat->coded_height > decodecaps.nMaxHeight)) {
        VPX_DPRINT("Resolution not supported on this GPU");
        return nDecodeSurface;
    }

    if ((pVideoFormat->coded_width >> 4) * (pVideoFormat->coded_height >> 4) >
        decodecaps.nMaxMBCount) {
        VPX_DPRINT("MBCount not supported on this GPU");
        return nDecodeSurface;
    }

    mLumaWidth =
            pVideoFormat->display_area.right - pVideoFormat->display_area.left;
    mLumaHeight =
            pVideoFormat->display_area.bottom - pVideoFormat->display_area.top;
    mChromaHeight = mLumaHeight * 0.5;  // NV12
    mBPP = pVideoFormat->bit_depth_luma_minus8 > 0 ? 2 : 1;

    CUVIDDECODECREATEINFO videoDecodeCreateInfo = {0};
    videoDecodeCreateInfo.CodecType = pVideoFormat->codec;
    videoDecodeCreateInfo.ChromaFormat = pVideoFormat->chroma_format;
    videoDecodeCreateInfo.OutputFormat = cudaVideoSurfaceFormat_NV12;
    VPX_DPRINT("output format is %d", videoDecodeCreateInfo.OutputFormat);
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
        VPX_DPRINT("old width %d old height %d", mOutputWidth, mOutputHeight);
        mOutputWidth = mLumaWidth;
        mOutputHeight = mLumaHeight;
        VPX_DPRINT("new width %d new height %d", mOutputWidth, mOutputHeight);
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
        VPX_DPRINT("free memory %g M, total %g M", free / 1048576.0,
                   total / 1048576.0);
    }
    NVDEC_API_CALL(cuCtxPopCurrent(NULL));
    NVDEC_API_CALL(cuCtxPushCurrent(mCudaContext));
    NVDEC_API_CALL(cuvidCreateDecoder(&mCudaDecoder, &videoDecodeCreateInfo));
    NVDEC_API_CALL(cuCtxPopCurrent(NULL));
    VPX_DPRINT("successfully called. decoder %p", mCudaDecoder);
    return nDecodeSurface;
}

int MediaVpxDecoderCuvid::HandlePictureDecode(CUVIDPICPARAMS* pPicParams) {
    NVDEC_API_CALL(cuvidDecodePicture(mCudaDecoder, pPicParams));
    VPX_DPRINT("successfully called.");
    return 1;
}

extern "C" {

#define MEDIA_Vpx_COPY_Y_TEXTURE 1
#define MEDIA_Vpx_COPY_UV_TEXTURE 2

struct vpx_cuvid_copy_context {
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

void vpx_cuda_copy_decoded_frame(void* privData,
                                 int mode,
                                 uint32_t dest_texture_handle) {
    vpx_cuvid_copy_context* copy_context =
            static_cast<vpx_cuvid_copy_context*>(privData);

    const unsigned int GL_TEXTURE_2D = 0x0DE1;
    const unsigned int cudaGraphicsMapFlagsNone = 0x0;
    CUgraphicsResource CudaRes{0};
    VPX_DPRINT("cuda copy decoded frame testure %d", (int)dest_texture_handle);
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
    VPX_DPRINT("dstPitch %d, WidthInBytes %d Height %d surface-height %d",
               (int)m.dstPitch, (int)m.WidthInBytes, (int)m.Height,
               (int)copy_context->src_surface_height);

    if (mode == MEDIA_Vpx_COPY_Y_TEXTURE) {  // copy Y data
        NVDEC_API_CALL(cuMemcpy2D(&m));
    } else if (mode == MEDIA_Vpx_COPY_UV_TEXTURE) {  // copy UV data
        m.srcDevice =
                (CUdeviceptr)((uint8_t*)dpSrcFrame +
                              m.srcPitch * copy_context->src_surface_height);
        m.Height = m.Height / 2;
        NVDEC_API_CALL(cuMemcpy2D(&m));
    }
    NVDEC_API_CALL(cuGraphicsUnmapResources(1, &CudaRes, 0));
    NVDEC_API_CALL(cuGraphicsUnregisterResource(CudaRes));
}

void vpx_cuda_nv12_updater(void* privData, uint32_t type, uint32_t* textures) {
    constexpr uint32_t kFRAMEWORK_FORMAT_NV12 = 3;
    if (type != kFRAMEWORK_FORMAT_NV12) {
        return;
    }
    VPX_DPRINT("copyiong Ytex %d", textures[0]);
    VPX_DPRINT("copyiong UVtex %d", textures[1]);
    vpx_cuda_copy_decoded_frame(privData, MEDIA_Vpx_COPY_Y_TEXTURE,
                                textures[0]);
    vpx_cuda_copy_decoded_frame(privData, MEDIA_Vpx_COPY_UV_TEXTURE,
                                textures[1]);
}

}  // end extern C

int MediaVpxDecoderCuvid::HandlePictureDisplay(CUVIDPARSERDISPINFO* pDispInfo) {

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
        vpx_cuvid_copy_context my_copy_context{
                .src_frame = dpSrcFrame,
                .src_pitch = nSrcPitch,
                .src_surface_height = mSurfaceHeight,
                .dest_width = mOutputWidth,
                .dest_height = mOutputHeight,
        };
        texFrame = mRenderer.getTextureFrame(mOutputWidth, mOutputHeight);
        mRenderer.saveDecodedFrameToTexture(texFrame, &my_copy_context,
                                            (void*)vpx_cuda_nv12_updater);
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
        VPX_DPRINT("dstDevice %p, dstPitch %d, WidthInBytes %d Height %d",
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
    {
        std::lock_guard<std::mutex> g(mFrameLock);
        mSavedFrames.push_back(myFrame);
        mSavedTexFrames.push_back(texFrame);
        mSavedPts.push_back(myOutputPts);
        mSavedW.push_back(mOutputWidth);
        mSavedH.push_back(mOutputHeight);
    }
    mImageReady = true;
    VPX_DPRINT("successfully called.");
    return 1;
}

bool MediaVpxDecoderCuvid::s_isCudaInitialized = false;
// static

}  // namespace emulation
}  // namespace android
