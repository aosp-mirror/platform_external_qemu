// Copyright (C) 2021 The Android Open Source Project
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

#include "host-common/MediaVideoToolBoxUtils.h"
#include "host-common/MediaVideoToolBoxVideoHelper.h"
#include "host-common/YuvConverter.h"
#include "android/utils/debug.h"

#define MEDIA_VTB_DEBUG 0

#if MEDIA_VTB_DEBUG
#define VTB_DPRINT(fmt, ...)                                             \
    fprintf(stderr, "media-vtb-video-helper: %s:%d " fmt "\n", __func__, \
            __LINE__, ##__VA_ARGS__);
#else
#define VTB_DPRINT(fmt, ...)
#endif

namespace android {
namespace emulation {

using TextureFrame = MediaTexturePool::TextureFrame;
using FrameInfo = MediaSnapshotState::FrameInfo;
using ColorAspects = MediaSnapshotState::ColorAspects;
using H264NaluType = H264NaluParser::H264NaluType;

MediaVideoToolBoxVideoHelper::MediaVideoToolBoxVideoHelper(
        int w,
        int h,
        OutputTreatmentMode oMode,
        FrameStorageMode fMode)
    : mOutputWidth(w),
      mOutputHeight(h),
      mUseGpuTexture(fMode == FrameStorageMode::USE_GPU_TEXTURE) {
    mIgnoreDecoderOutput = (oMode == OutputTreatmentMode::IGNORE_RESULT);
}

MediaVideoToolBoxVideoHelper::~MediaVideoToolBoxVideoHelper() {
    deInit();
}

void MediaVideoToolBoxVideoHelper::deInit() {
    VTB_DPRINT("deInit calling");

    resetDecoderSession();

    resetFormatDesc();

    mVtbReady = false;

    mFfmpegVideoHelper.reset();

    mSavedDecodedFrames.clear();
}

bool MediaVideoToolBoxVideoHelper::init() {
    VTB_DPRINT("init calling");
    mVtbReady = false;
    return true;
}

static void dumpBytes(const uint8_t* img, size_t szBytes, bool all = false) {
    printf("data=");
    size_t numBytes = szBytes;
    if (!all) {
        numBytes = 32;
    }

    for (size_t i = 0; i < (numBytes > szBytes ? szBytes : numBytes); ++i) {
        if (i % 8 == 0) {
            printf("\n");
        }
        printf("0x%02x ", img[i]);
    }
    printf("\n");

    fflush(stdout);
}

void MediaVideoToolBoxVideoHelper::extractFrameInfo() {
    // at this moment, ffmpeg should have decoded the first frame to obtain
    // w/h/colorspace info and number of b frame buffers

    mFfmpegVideoHelper->flush();
    MediaSnapshotState::FrameInfo frame;
    bool success = mFfmpegVideoHelper->receiveFrame(&frame);
    if (success) {
        mColorAspects = frame.color;
        mVtbBufferSize = mFfmpegVideoHelper->frameReorderBufferSize();
        VTB_DPRINT("vtb buffer size is %d", mVtbBufferSize);
    }
}

void MediaVideoToolBoxVideoHelper::decode(const uint8_t* frame,
                                          size_t szBytes,
                                          uint64_t inputPts) {
    VTB_DPRINT("%s(frame=%p, sz=%zu)", __func__, frame, szBytes);

    ++mTotalFrames;
    const bool parseOk = parseInputFrames(frame, szBytes);
    if (!parseOk) {
        // cannot parse, probably cannot decode either
        // just fail
        VTB_DPRINT("Failed to parse frame=%p, sz=%zu, give up.", frame, szBytes);
        mIsGood = false;
        return;
    }

    // has to go in the FIFO order
    for (int i = 0; i < mInputFrames.size(); ++i) {
        InputFrame& f = mInputFrames[i];
        switch (f.type) {
            case H264NaluType::SPS:
                mSPS.assign(f.data, f.data + f.size);
                VTB_DPRINT("this is an SPS frame");
                // dumpBytes(mSPS.data(), mSPS.size());
                mVtbReady = false;
                {   // create ffmpeg decoder with only 1 thread to avoid frame delay
                    constexpr int numthreads = 1;
                    mFfmpegVideoHelper.reset(new MediaFfmpegVideoHelper(264, numthreads));
                    mFfmpegVideoHelper->init();
                }
                break;
            case H264NaluType::PPS:
                VTB_DPRINT("this is an PPS frame");
                mPPS.assign(f.data, f.data + f.size);
                // dumpBytes(mPPS.data(), mPPS.size());
                mVtbReady = false;
                break;
            case H264NaluType::SEI:
                VTB_DPRINT("this is SEI frame");
                break;
            case H264NaluType::CodedSliceIDR:
                VTB_DPRINT("this is an IDR frame");
                if (mFfmpegVideoHelper) {
                    mFfmpegVideoHelper->decode(frame, szBytes, inputPts);
                    extractFrameInfo();
                    mFfmpegVideoHelper.reset();
                }
                if (!mVtbReady) {
                    createCMFormatDescription();
                    Boolean needNewSession = ( VTDecompressionSessionCanAcceptFormatDescription(mDecoderSession, mCmFmtDesc) == false);
                    if (needNewSession) {
                        resetDecoderSession();
                    }
                    if (!mDecoderSession) {
                        // all the previously decoded frames need to be
                        // retrieved
                        flush();
                        recreateDecompressionSession();
                    }
                    if (mIsGood) {
                        mVtbReady = true;
                    }
                }
                handleIDRFrame(f.data, f.size, inputPts);
                break;
            case H264NaluType::CodedSliceNonIDR:
                VTB_DPRINT("this is an non-IDR frame");
                handleIDRFrame(f.data, f.size, inputPts);
                break;
            default:
                VTB_DPRINT("Support for nalu_type=%u not implemented",
                           (uint8_t)f.type);
                // cannot handle such video, give up so we can fall
                // back to ffmpeg
                mIsGood = false;
                break;
        }
    }

    if (mFfmpegVideoHelper) {
        // we have not reached idr frame yet, keep decoding
        mFfmpegVideoHelper->decode(frame, szBytes, inputPts);
    }

    mInputFrames.clear();
}

void MediaVideoToolBoxVideoHelper::flush() {
    VTB_DPRINT("started flushing");
    for (auto& iter : mVtbBufferMap) {
        mSavedDecodedFrames.push_back(std::move(iter.second));
    }
    mVtbBufferMap.clear();
    VTB_DPRINT("done one flushing");
}

//------------------------  private  helper functions
//-----------------------------------------

void MediaVideoToolBoxVideoHelper::handleIDRFrame(const uint8_t* ptr,
                                                  size_t szBytes,
                                                  uint64_t pts) {
    uint8_t* fptr = const_cast<uint8_t*>(ptr);

    // We can assume fptr has a valid start code header because it has already
    // gone through validation in H264NaluParser.
    uint8_t startHeaderSz = fptr[2] == 1 ? 3 : 4;
    uint32_t dataSz = szBytes - startHeaderSz;
    std::unique_ptr<uint8_t> idr(new uint8_t[dataSz + 4]);
    uint32_t dataSzNl = htonl(dataSz);

    // AVCC format requires us to replace the start code header on this NALU
    // with the size of the data. Start code is either 0x000001 or 0x00000001.
    // The size needs to be the first four bytes in network byte order.
    memcpy(idr.get(), &dataSzNl, 4);
    memcpy(idr.get() + 4, ptr + startHeaderSz, dataSz);

    CMSampleBufferRef sampleBuf = nullptr;
    sampleBuf = createSampleBuffer(mCmFmtDesc, (void*)idr.get(), dataSz + 4);
    if (!sampleBuf) {
        VTB_DPRINT("%s: Failed to create CMSampleBufferRef", __func__);
        return;
    }

    CMSampleBufferSetOutputPresentationTimeStamp(sampleBuf, CMTimeMake(pts, 1));

    OSStatus status;
    status = VTDecompressionSessionDecodeFrame(mDecoderSession, sampleBuf,
                                               0,     // decodeFlags
                                               NULL,  // sourceFrameRefCon
                                               0);    // infoFlagsOut

    if (status == noErr) {
        // TODO: this call blocks until the frame has been decoded. Perhaps it
        // will be more efficient to signal the guest when the frame is ready to
        // be read instead.
        status = VTDecompressionSessionWaitForAsynchronousFrames(
                mDecoderSession);
        VTB_DPRINT("Success decoding IDR frame");
    } else {
        VTB_DPRINT("%s: Failed to decompress frame (err=%d)", __func__, status);
    }

    CFRelease(sampleBuf);
}

// chop the frame into sub frames that video tool box will consume
// one by one
bool MediaVideoToolBoxVideoHelper::parseInputFrames(const uint8_t* frame,
                                                    size_t sz) {
    mInputFrames.clear();
    VTB_DPRINT("input frame %d bytes", (int)sz);
    while (1) {
        const uint8_t* remainingFrame = parseOneFrame(frame, sz);
        if (remainingFrame == nullptr)
            break;
        int consumed = (remainingFrame - frame);
        VTB_DPRINT("consumed %d bytes", consumed);
        frame = remainingFrame;
        sz = sz - consumed;
    }
    return mInputFrames.size() > 0;
}

const uint8_t* MediaVideoToolBoxVideoHelper::parseOneFrame(const uint8_t* frame,
                                                           size_t szBytes) {
    if (frame == nullptr || szBytes <= 0) {
        return nullptr;
    }

    const uint8_t* currentNalu =
            H264NaluParser::getNextStartCodeHeader(frame, szBytes);
    if (currentNalu == nullptr) {
        VTB_DPRINT("No start code header found in this frame");
        return nullptr;
    }

    const uint8_t* nextNalu = nullptr;
    size_t remaining = szBytes - (currentNalu - frame);

    // Figure out the size of |currentNalu|.
    size_t currentNaluSize = remaining;
    // 3 is the minimum size of the start code header (3 or 4 bytes).
    // dumpBytes(currentNalu, currentNaluSize);

    // usually guest sends one frame a time, but sometime, SEI prefixes IDR
    // frame as one frame from guest, we need to separate SEI out from IDR:
    // video tool box cannot handle SEI+IDR combo-frame; for other cases, there
    // is no need to check.
    if (H264NaluType::SEI == H264NaluParser::getFrameNaluType(frame, szBytes, nullptr)
     || H264NaluType::SPS == H264NaluParser::getFrameNaluType(frame, szBytes, nullptr)
     || H264NaluType::PPS == H264NaluParser::getFrameNaluType(frame, szBytes, nullptr)

        ) {
        nextNalu = H264NaluParser::getNextStartCodeHeader(currentNalu + 3,
                                                          remaining - 3);
        if (nextNalu != nullptr) {
            currentNaluSize = nextNalu - currentNalu;
        }
    }

    const uint8_t* remainingFrame = currentNalu + currentNaluSize;

    // |data| is currentNalu, but with the start code header discarded.
    uint8_t* data = nullptr;
    H264NaluType naluType = H264NaluParser::getFrameNaluType(
            currentNalu, currentNaluSize, &data);

    if (naluType == H264NaluType::Undefined)
        return nullptr;

    size_t dataSize = currentNaluSize - (data - currentNalu);
    const std::string naluTypeStr = H264NaluParser::naluTypeToString(naluType);
    VTB_DPRINT("Got frame type=%u (%s)", (uint8_t)naluType,
               naluTypeStr.c_str());

    if (naluType == H264NaluType::SPS || naluType == H264NaluType::PPS) {
        mInputFrames.push_back(InputFrame(naluType, data, dataSize));
    } else {
        mInputFrames.push_back(
                InputFrame(naluType, currentNalu, currentNaluSize));
    }
    return remainingFrame;
}

// static
void MediaVideoToolBoxVideoHelper::videoToolboxDecompressCallback(
        void* opaque,
        void* sourceFrameRefCon,
        OSStatus status,
        VTDecodeInfoFlags flags,
        CVPixelBufferRef image_buffer,
        CMTime pts,
        CMTime duration) {
    VTB_DPRINT("%s", __func__);
    auto ptr = static_cast<MediaVideoToolBoxVideoHelper*>(opaque);

    // if (ptr->mDecodedFrame) {
    //     CVPixelBufferRelease(ptr->mDecodedFrame);
    //     ptr->mDecodedFrame = nullptr;
    // }

    if (!image_buffer) {
        VTB_DPRINT("%s: output image buffer is null", __func__);
        ptr->mIsGood = false;
        return;
    }

    ptr->mOutputPts = pts.value;
    ptr->mDecodedFrame = CVPixelBufferRetain(image_buffer);
    CVOpenGLTextureCacheRef _CVGLTextureCache;
    CVOpenGLTextureRef _CVGLTexture;
    CGLPixelFormatObj _CGLPixelFormat;

    // Image is ready to be comsumed
    ptr->copyFrame();
    ptr->mImageReady = true;
    VTB_DPRINT("Got decoded frame");
    CVPixelBufferRelease(ptr->mDecodedFrame);
    ptr->mDecodedFrame = nullptr;
}

// static
CFDictionaryRef MediaVideoToolBoxVideoHelper::createOutputBufferAttributes(
        int width,
        int height,
        OSType pix_fmt) {
    CFMutableDictionaryRef buffer_attributes;
    CFMutableDictionaryRef io_surface_properties;
    CFNumberRef cv_pix_fmt;
    CFNumberRef w;
    CFNumberRef h;

    w = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &width);
    h = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &height);
    cv_pix_fmt =
            CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pix_fmt);

    buffer_attributes = CFDictionaryCreateMutable(
            kCFAllocatorDefault, 4, &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);
    io_surface_properties = CFDictionaryCreateMutable(
            kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);

    if (pix_fmt) {
        CFDictionarySetValue(buffer_attributes,
                             kCVPixelBufferPixelFormatTypeKey, cv_pix_fmt);
    }
    CFDictionarySetValue(buffer_attributes,
                         kCVPixelBufferIOSurfacePropertiesKey,
                         io_surface_properties);
    CFDictionarySetValue(buffer_attributes, kCVPixelBufferWidthKey, w);
    CFDictionarySetValue(buffer_attributes, kCVPixelBufferHeightKey, h);
    // Not sure if this will work becuase we are passing the pixel buffer back
    // into the guest
    CFDictionarySetValue(buffer_attributes,
                         kCVPixelBufferIOSurfaceOpenGLTextureCompatibilityKey,
                         kCFBooleanTrue);

    CFRelease(io_surface_properties);
    CFRelease(cv_pix_fmt);
    CFRelease(w);
    CFRelease(h);

    return buffer_attributes;
}

// static
CMSampleBufferRef MediaVideoToolBoxVideoHelper::createSampleBuffer(
        CMFormatDescriptionRef fmtDesc,
        void* buffer,
        size_t sz) {
    OSStatus status;
    CMBlockBufferRef blockBuf = nullptr;
    CMSampleBufferRef sampleBuf = nullptr;

    status = CMBlockBufferCreateWithMemoryBlock(
            kCFAllocatorDefault,  // structureAllocator
            buffer,               // memoryBlock
            sz,                   // blockLength
            kCFAllocatorNull,     // blockAllocator
            NULL,                 // customBlockSource
            0,                    // offsetToData
            sz,                   // dataLength
            0,                    // flags
            &blockBuf);

    if (!status) {
        status = CMSampleBufferCreate(kCFAllocatorDefault,  // allocator
                                      blockBuf,             // dataBuffer
                                      TRUE,                 // dataReady
                                      0,        // makeDataReadyCallback
                                      0,        // makeDataReadyRefCon
                                      fmtDesc,  // formatDescription
                                      1,        // numSamples
                                      0,        // numSampleTimingEntries
                                      NULL,     // sampleTimingArray
                                      0,        // numSampleSizeEntries
                                      NULL,     // sampleSizeArray
                                      &sampleBuf);
    }

    if (blockBuf) {
        CFRelease(blockBuf);
    }

    return sampleBuf;
}

void MediaVideoToolBoxVideoHelper::resetDecoderSession() {
    if (mDecoderSession) {
        VTDecompressionSessionInvalidate(mDecoderSession);
        CFRelease(mDecoderSession);
        mDecoderSession = nullptr;
    }
    if (mDecodedFrame) {
        CVPixelBufferRelease(mDecodedFrame);
        mDecodedFrame = nullptr;
    }
}

void MediaVideoToolBoxVideoHelper::getOutputWH() {
    if (!mCmFmtDesc)
        return;

    CMVideoDimensions vsize = CMVideoFormatDescriptionGetDimensions(mCmFmtDesc);

    if (mOutputWidth == vsize.width && mOutputHeight == vsize.height) {
        VTB_DPRINT("initial w/h is the same as vtb w/h");
    } else {
        VTB_DPRINT("initial w/h are not the same as vtb w/h");
    }
    mOutputWidth = vsize.width;
    mOutputHeight = vsize.height;
}

void MediaVideoToolBoxVideoHelper::resetFormatDesc() {
    if (mCmFmtDesc) {
        CFRelease(mCmFmtDesc);
        mCmFmtDesc = nullptr;
    }
}

void MediaVideoToolBoxVideoHelper::createCMFormatDescription() {
    uint8_t* parameterSets[2] = {mSPS.data(), mPPS.data()};
    size_t parameterSetSizes[2] = {mSPS.size(), mPPS.size()};

    resetFormatDesc();

    OSStatus status = CMVideoFormatDescriptionCreateFromH264ParameterSets(
            kCFAllocatorDefault, 2, (const uint8_t* const*)parameterSets,
            parameterSetSizes, 4, &mCmFmtDesc);

    if (status == noErr) {
        getOutputWH();
        VTB_DPRINT("Created CMFormatDescription from SPS/PPS sets w %d h %d",
                   mOutputWidth, mOutputHeight);
    } else {
        VTB_DPRINT("Unable to create CMFormatDescription (%d)\n", (int)status);
    }
}

void MediaVideoToolBoxVideoHelper::recreateDecompressionSession() {
    if (mCmFmtDesc == nullptr) {
        VTB_DPRINT("CMFormatDescription not created. Need sps and pps NALUs.");
        return;
    }

    CMVideoCodecType codecType = kCMVideoCodecType_H264;
    CFMutableDictionaryRef decoder_spec = CFDictionaryCreateMutable(
            kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks,
            &kCFTypeDictionaryValueCallBacks);
    // enable hardware acceleration, but allows vtb to fallback to
    // its software implementation.
    CFDictionarySetValue(
            decoder_spec,
            kVTVideoDecoderSpecification_EnableHardwareAcceleratedVideoDecoder,
            kCFBooleanTrue);

    CFDictionaryRef bufAttr;

    if (mUseGpuTexture) {
        // use NV12 format
        bufAttr = createOutputBufferAttributes(
                mOutputWidth, mOutputHeight,
                kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange);
    } else {
        bufAttr = createOutputBufferAttributes(
                mOutputWidth, mOutputHeight,
                kCVPixelFormatType_420YpCbCr8Planar);
    }

    VTDecompressionOutputCallbackRecord decoderCb;
    decoderCb.decompressionOutputCallback = videoToolboxDecompressCallback;
    decoderCb.decompressionOutputRefCon = this;

    OSStatus status;
    status = VTDecompressionSessionCreate(
            NULL,               // allocator
            mCmFmtDesc,         // videoFormatDescription
            decoder_spec,       // videoDecoderSpecification
            bufAttr,            // destinationImageBufferAttributes
            &decoderCb,         // outputCallback
            &mDecoderSession);  // decompressionSessionOut

    if (decoder_spec) {
        CFRelease(decoder_spec);
    }
    if (bufAttr) {
        CFRelease(bufAttr);
    }

    mIsGood = false;
    switch (status) {
        case kVTVideoDecoderNotAvailableNowErr:
            VTB_DPRINT("VideoToolbox session not available");
            return;
        case kVTVideoDecoderUnsupportedDataFormatErr:
            VTB_DPRINT("VideoToolbox does not support this format");
            return;
        case kVTVideoDecoderMalfunctionErr:
            VTB_DPRINT("VideoToolbox malfunction");
            return;
        case kVTVideoDecoderBadDataErr:
            VTB_DPRINT("VideoToolbox reported invalid data");
            return;
        case 0:
            dprint("VideoToolBox decoding session is created successfully");
            mIsGood = true;
            return;
        default:
            VTB_DPRINT("Unknown VideoToolbox session creation error %d",
                       status);
            return;
    }
}

void MediaVideoToolBoxVideoHelper::copyFrameToTextures() {
    // TODO
    auto startTime = std::chrono::steady_clock::now();
    TextureFrame texFrame;
    if (mUseGpuTexture && mTexturePool != nullptr) {
        VTB_DPRINT("decoded surface is %p w %d h %d",
                   CVPixelBufferGetIOSurface(mDecodedFrame), mOutputWidth,
                   mOutputHeight);
        auto my_copy_context = media_vtb_utils_copy_context{
                CVPixelBufferGetIOSurface(mDecodedFrame), mOutputWidth,
                mOutputHeight};
        texFrame = mTexturePool->getTextureFrame(mOutputWidth, mOutputHeight);
        VTB_DPRINT("ask videocore to copy to %d and %d", texFrame.Ytex,
                   texFrame.UVtex);
        mTexturePool->saveDecodedFrameToTexture(
                texFrame, &my_copy_context,
                (void*)media_vtb_utils_nv12_updater);
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime);
    VTB_DPRINT("used %d ms", (int)elapsed.count());

    auto ptspair = std::make_pair(mOutputPts, mTotalFrames);
    mVtbBufferMap[ptspair] = MediaSnapshotState::FrameInfo{
            std::vector<uint8_t>(),
            std::vector<uint32_t>{texFrame.Ytex, texFrame.UVtex},
            (int)mOutputWidth,
            (int)mOutputHeight,
            (uint64_t)(mOutputPts),
            ColorAspects{mColorAspects}};
}

void MediaVideoToolBoxVideoHelper::copyFrameToCPU() {
    int imageSize = CVPixelBufferGetDataSize(mDecodedFrame);
    int stride = CVPixelBufferGetBytesPerRow(mDecodedFrame);

    mOutBufferSize = mOutputWidth * mOutputHeight * 3 / 2;

    VTB_DPRINT("copying size=%d dimension=[%dx%d] stride=%d", imageSize,
               mOutputWidth, mOutputHeight, stride);

    // Copies the image data to the guest.
    mSavedDecodedFrame.resize(mOutputWidth * mOutputHeight * 3 / 2);
    uint8_t* dst = mSavedDecodedFrame.data();

    CVPixelBufferLockBaseAddress(mDecodedFrame, kCVPixelBufferLock_ReadOnly);
    if (CVPixelBufferIsPlanar(mDecodedFrame)) {
        imageSize = 0;  // add up the size from the planes
        int planes = CVPixelBufferGetPlaneCount(mDecodedFrame);
        for (int i = 0; i < planes; ++i) {
            void* planeData =
                    CVPixelBufferGetBaseAddressOfPlane(mDecodedFrame, i);
            int linesize = CVPixelBufferGetBytesPerRowOfPlane(mDecodedFrame, i);
            int planeWidth = CVPixelBufferGetWidthOfPlane(mDecodedFrame, i);
            int planeHeight = CVPixelBufferGetHeightOfPlane(mDecodedFrame, i);
            VTB_DPRINT("plane=%d data=%p linesize=%d pwidth=%d pheight=%d", i,
                       planeData, linesize, planeWidth, planeHeight);
            // For kCVPixelFormatType_420YpCbCr8Planar, plane 0 is Y, UV planes
            // are 1 and 2
            if (planeWidth != mOutputWidth && planeWidth != mOutputWidth / 2) {
                VTB_DPRINT("ERROR: Unable to determine YUV420 plane type");
                continue;
            }

            // Sometimes the buffer stride can be longer than the actual data
            // width. This means that the extra bytes are just padding and need
            // to be discarded.
            if (linesize <= planeWidth) {
                int sz = planeHeight * planeWidth;
                imageSize += sz;
                memcpy(dst, planeData, sz);
                dst += sz;
            } else {
                // Need to copy line by line
                int sz = planeWidth;
                for (int j = 0; j < planeHeight; ++j) {
                    uint8_t* ptr = (uint8_t*)planeData;
                    ptr += linesize * j;
                    memcpy(dst, ptr, sz);
                    imageSize += sz;
                    dst += sz;
                }
            }
        }
        if (imageSize != mOutBufferSize) {
            VTB_DPRINT(
                    "ERROR: Total size of planes not same as guestSz "
                    "(guestSz=%u, imageSize=%d)",
                    mOutBufferSize, imageSize);
        }
    } else {
        if (imageSize > mOutBufferSize) {
            VTB_DPRINT(
                    "Buffer size mismatch (guestSz=%u, imageSize=%d). Using "
                    "guestSz instead.",
                    mOutBufferSize, imageSize);
            imageSize = mOutBufferSize;
        }

        // IMPORTANT: mDecodedFrame must be locked before accessing the contents
        // with CPU
        void* data = CVPixelBufferGetBaseAddress(mDecodedFrame);
        memcpy(dst, data, imageSize);
    }
    CVPixelBufferUnlockBaseAddress(mDecodedFrame, kCVPixelBufferLock_ReadOnly);

    auto ptspair = std::make_pair(mOutputPts, mTotalFrames);
    mVtbBufferMap[ptspair] = MediaSnapshotState::FrameInfo{
            std::vector<uint8_t>(), std::vector<uint32_t>{},
            (int)mOutputWidth,      (int)mOutputHeight,
            (uint64_t)(mOutputPts), ColorAspects{mColorAspects}};
    mVtbBufferMap[ptspair].data.swap(mSavedDecodedFrame);
}

void MediaVideoToolBoxVideoHelper::copyFrame() {
    mOutputWidth = CVPixelBufferGetWidth(mDecodedFrame);
    mOutputHeight = CVPixelBufferGetHeight(mDecodedFrame);

    if (mUseGpuTexture) {
        copyFrameToTextures();
    } else {
        copyFrameToCPU();
    }

    if (mVtbBufferMap.size() > mVtbBufferSize) {
        mSavedDecodedFrames.push_back(std::move(mVtbBufferMap.begin()->second));
        mVtbBufferMap.erase(mVtbBufferMap.begin());
    }
}

}  // namespace emulation
}  // namespace android
