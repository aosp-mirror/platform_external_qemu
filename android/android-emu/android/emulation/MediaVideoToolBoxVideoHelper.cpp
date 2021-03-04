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

#include "android/emulation/MediaVideoToolBoxVideoHelper.h"
#include "android/emulation/YuvConverter.h"
#include "android/utils/debug.h"

extern "C" {
}

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
using PixelFormat = MediaH264Decoder::PixelFormat;

MediaVideoToolBoxVideoHelper::MediaVideoToolBoxVideoHelper(OutputTreatmentMode oMode,
                                           FrameStorageMode fMode)
    : mUseGpuTexture(fMode == FrameStorageMode::USE_GPU_TEXTURE) {
    mIgnoreDecoderOutput = (oMode == OutputTreatmentMode::IGNORE_RESULT);
}

MediaVideoToolBoxVideoHelper::~MediaVideoToolBoxVideoHelper() {
    deInit();
}

void MediaVideoToolBoxVideoHelper::deInit() {
    VTB_DPRINT("deInit calling");

    mSavedDecodedFrames.clear();
}

bool MediaVideoToolBoxVideoHelper::init() {
    return true;
}

void MediaVideoToolBoxVideoHelper::decode(const uint8_t* frame,
                                  size_t szBytes,
                                  uint64_t inputPts) {
    VTB_DPRINT("%s(frame=%p, sz=%zu)", __func__, frame, szBytes);

}

void MediaVideoToolBoxVideoHelper::flush() {
    VTB_DPRINT("started flushing");
    VTB_DPRINT("done one flushing");
}

// static
void MediaVideoToolBoxVideoHelper::videoToolboxDecompressCallback(void* opaque,
                                                          void* sourceFrameRefCon,
                                                          OSStatus status,
                                                          VTDecodeInfoFlags flags,
                                                          CVImageBufferRef image_buffer,
                                                          CMTime pts,
                                                          CMTime duration) {
    VTB_DPRINT("%s", __func__);
    auto ptr = static_cast<MediaVideoToolBoxVideoHelper*>(opaque);

    if (ptr->mDecodedFrame) {
        CVPixelBufferRelease(ptr->mDecodedFrame);
        ptr->mDecodedFrame = nullptr;
    }

    if (!image_buffer) {
        VTB_DPRINT("%s: output image buffer is null", __func__);
        return;
    }

    ptr->mOutputPts = pts.value;
    ptr->mDecodedFrame = CVPixelBufferRetain(image_buffer);
    // Image is ready to be comsumed
    ptr->copyFrame();
    ptr->mImageReady = true;
    VTB_DPRINT("Got decoded frame");
}

// static
CFDictionaryRef MediaVideoToolBoxVideoHelper::createOutputBufferAttributes(int width,
                                                                   int height,
                                                                   OSType pix_fmt) {
    CFMutableDictionaryRef buffer_attributes;
    CFMutableDictionaryRef io_surface_properties;
    CFNumberRef cv_pix_fmt;
    CFNumberRef w;
    CFNumberRef h;

    w = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &width);
    h = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &height);
    cv_pix_fmt = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pix_fmt);

    buffer_attributes = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                  4,
                                                  &kCFTypeDictionaryKeyCallBacks,
                                                  &kCFTypeDictionaryValueCallBacks);
    io_surface_properties = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                      0,
                                                      &kCFTypeDictionaryKeyCallBacks,
                                                      &kCFTypeDictionaryValueCallBacks);

    if (pix_fmt) {
        CFDictionarySetValue(buffer_attributes, kCVPixelBufferPixelFormatTypeKey, cv_pix_fmt);
    }
    CFDictionarySetValue(buffer_attributes, kCVPixelBufferIOSurfacePropertiesKey, io_surface_properties);
    CFDictionarySetValue(buffer_attributes, kCVPixelBufferWidthKey, w);
    CFDictionarySetValue(buffer_attributes, kCVPixelBufferHeightKey, h);
    // Not sure if this will work becuase we are passing the pixel buffer back into the guest
    CFDictionarySetValue(buffer_attributes, kCVPixelBufferIOSurfaceOpenGLTextureCompatibilityKey, kCFBooleanTrue);

    CFRelease(io_surface_properties);
    CFRelease(cv_pix_fmt);
    CFRelease(w);
    CFRelease(h);

    return buffer_attributes;
}

// static
CMSampleBufferRef MediaVideoToolBoxVideoHelper::createSampleBuffer(CMFormatDescriptionRef fmtDesc,
                                                           void* buffer,
                                                           size_t sz) {
    OSStatus status;
    CMBlockBufferRef blockBuf = nullptr;
    CMSampleBufferRef sampleBuf = nullptr;

    status = CMBlockBufferCreateWithMemoryBlock(kCFAllocatorDefault, // structureAllocator
                                                buffer,              // memoryBlock
                                                sz,                  // blockLength
                                                kCFAllocatorNull,    // blockAllocator
                                                NULL,                // customBlockSource
                                                0,                   // offsetToData
                                                sz,                  // dataLength
                                                0,                   // flags
                                                &blockBuf);

    if (!status) {
        status = CMSampleBufferCreate(kCFAllocatorDefault, // allocator
                                      blockBuf,            // dataBuffer
                                      TRUE,                // dataReady
                                      0,                   // makeDataReadyCallback
                                      0,                   // makeDataReadyRefCon
                                      fmtDesc,             // formatDescription
                                      1,                   // numSamples
                                      0,                   // numSampleTimingEntries
                                      NULL,                // sampleTimingArray
                                      0,                   // numSampleSizeEntries
                                      NULL,                // sampleSizeArray
                                      &sampleBuf);
    }

    if (blockBuf) {
        CFRelease(blockBuf);
    }

    return sampleBuf;
}

// static
OSType MediaVideoToolBoxVideoHelper::toNativePixelFormat(PixelFormat pixFmt) {
    switch (pixFmt) {
        case PixelFormat::YUV420P:
            return kCVPixelFormatType_420YpCbCr8Planar;
        case PixelFormat::UYVY422:
            return kCVPixelFormatType_422YpCbCr8;
        case PixelFormat::BGRA8888:
            return kCVPixelFormatType_32BGRA;
        default:
            VTB_DPRINT("Unsupported VideoToolbox pixel format");
            return '0000';
    }
}

void MediaVideoToolBoxVideoHelper::recreateDecompressionSession() {
    if (mCmFmtDesc == nullptr) {
        VTB_DPRINT("CMFormatDescription not created. Need sps and pps NALUs.");
        return;
    }

    // Create a new VideoToolbox decoder session if one already exists
    if (mDecoderSession != nullptr) {
        // TODO: Once we implement async frame readback, we'll need to flush all of the frames here and
        // store them somewhere for the guest to read later.
        VTDecompressionSessionInvalidate(mDecoderSession);
        CFRelease(mDecoderSession);
        mDecoderSession = nullptr;
        if (mDecodedFrame) {
            CVPixelBufferRelease(mDecodedFrame);
            mDecodedFrame = nullptr;
        }
    }

    CMVideoCodecType codecType = kCMVideoCodecType_H264;
    CFMutableDictionaryRef decoder_spec = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                                    0,
                                                                    &kCFTypeDictionaryKeyCallBacks,
                                                                    &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(decoder_spec,
                         kVTVideoDecoderSpecification_RequireHardwareAcceleratedVideoDecoder,
                         kCFBooleanTrue);

    CFDictionaryRef bufAttr = createOutputBufferAttributes(mOutputWidth,
                                                           mOutputHeight,
                                                           toNativePixelFormat(mOutPixFmt));

    VTDecompressionOutputCallbackRecord decoderCb;
    decoderCb.decompressionOutputCallback = videoToolboxDecompressCallback;
    decoderCb.decompressionOutputRefCon = this;

    OSStatus status;
    status = VTDecompressionSessionCreate(NULL,              // allocator
                                          mCmFmtDesc,        // videoFormatDescription
                                          decoder_spec,      // videoDecoderSpecification
                                          bufAttr,           // destinationImageBufferAttributes
                                          &decoderCb,        // outputCallback
                                          &mDecoderSession); // decompressionSessionOut

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
            VTB_DPRINT("VideoToolbox session created");
            mIsGood = true;
            return;
        default:
            VTB_DPRINT("Unknown VideoToolbox session creation error %d", status);
            return;
    }
}

void MediaVideoToolBoxVideoHelper::copyFrame() {

    int imgWidth = CVPixelBufferGetWidth(mDecodedFrame);
    int imgHeight = CVPixelBufferGetHeight(mDecodedFrame);
    int imageSize = CVPixelBufferGetDataSize(mDecodedFrame);
    int stride = CVPixelBufferGetBytesPerRow(mDecodedFrame);

    mOutputWidth = imgWidth;
    mOutputHeight = imgHeight;
    mOutBufferSize = mOutputWidth * mOutputHeight * 3 / 2;

    VTB_DPRINT("copying size=%d dimension=[%dx%d] stride=%d", imageSize, imgWidth, imgHeight, stride);

    // Copies the image data to the guest.
    mSavedDecodedFrame.resize(imgWidth * imgHeight * 3 / 2);
    uint8_t* dst = mSavedDecodedFrame.data();

    CVPixelBufferLockBaseAddress(mDecodedFrame, kCVPixelBufferLock_ReadOnly);
    if (CVPixelBufferIsPlanar(mDecodedFrame)) {
        imageSize = 0; // add up the size from the planes
        int planes = CVPixelBufferGetPlaneCount(mDecodedFrame);
        for (int i = 0; i < planes; ++i) {
            void* planeData = CVPixelBufferGetBaseAddressOfPlane(mDecodedFrame, i);
            int linesize = CVPixelBufferGetBytesPerRowOfPlane(mDecodedFrame, i);
            int planeWidth = CVPixelBufferGetWidthOfPlane(mDecodedFrame, i);
            int planeHeight = CVPixelBufferGetHeightOfPlane(mDecodedFrame, i);
            VTB_DPRINT("plane=%d data=%p linesize=%d pwidth=%d pheight=%d", i, planeData, linesize, planeWidth, planeHeight);
            // For kCVPixelFormatType_420YpCbCr8Planar, plane 0 is Y, UV planes are 1 and 2
            if (planeWidth != imgWidth && planeWidth != imgWidth / 2) {
                VTB_DPRINT("ERROR: Unable to determine YUV420 plane type");
                continue;
            }

            // Sometimes the buffer stride can be longer than the actual data width. This means that
            // the extra bytes are just padding and need to be discarded.
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
            VTB_DPRINT("ERROR: Total size of planes not same as guestSz (guestSz=%u, imageSize=%d)", mOutBufferSize, imageSize);
        }
    } else {
        if (imageSize > mOutBufferSize) {
            VTB_DPRINT("Buffer size mismatch (guestSz=%u, imageSize=%d). Using guestSz instead.", mOutBufferSize, imageSize);
            imageSize = mOutBufferSize;
        }

        // IMPORTANT: mDecodedFrame must be locked before accessing the contents with CPU
        void* data = CVPixelBufferGetBaseAddress(mDecodedFrame);
        memcpy(dst, data, imageSize);
    }
    CVPixelBufferUnlockBaseAddress(mDecodedFrame, kCVPixelBufferLock_ReadOnly);
}

}  // namespace emulation
}  // namespace android
