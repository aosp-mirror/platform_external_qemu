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

#include "android/emulation/MediaH264DecoderVideoToolBox.h"

#include "android/emulation/H264NaluParser.h"

#include <VideoToolbox/VideoToolbox.h>

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#ifndef kVTVideoDecoderSpecification_RequireHardwareAcceleratedVideoDecoder
#define kVTVideoDecoderSpecification_RequireHardwareAcceleratedVideoDecoder CFSTR("RequireHardwareAcceleratedVideoDecoder")
#endif

#define MEDIA_H264_DEBUG 0

#if MEDIA_H264_DEBUG
#define H264_DPRINT(fmt,...) fprintf(stderr, "h264-videotoolbox-dec: %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define H264_DPRINT(fmt,...)
#endif


namespace android {
namespace emulation {

using InitContextParam = H264PingInfoParser::InitContextParam;
using DecodeFrameParam = H264PingInfoParser::DecodeFrameParam;
using ResetParam = H264PingInfoParser::ResetParam;
using GetImageParam = H264PingInfoParser::GetImageParam;
using H264NaluType = H264NaluParser::H264NaluType;

MediaH264DecoderVideoToolBox::MediaH264DecoderVideoToolBox(
        uint64_t id,
        H264PingInfoParser parser)
    : mId(id), mParser(parser) {
    H264_DPRINT("created MediaH264DecoderVideoToolBox %p", this);
}

MediaH264DecoderPlugin* MediaH264DecoderVideoToolBox::clone() {
    return new MediaH264DecoderVideoToolBox(mId, mParser);
}

MediaH264DecoderVideoToolBox::~MediaH264DecoderVideoToolBox() {
    destroyH264Context();
}

// static
void MediaH264DecoderVideoToolBox::videoToolboxDecompressCallback(void* opaque,
                                                          void* sourceFrameRefCon,
                                                          OSStatus status,
                                                          VTDecodeInfoFlags flags,
                                                          CVImageBufferRef image_buffer,
                                                          CMTime pts,
                                                          CMTime duration) {
    H264_DPRINT("%s", __func__);
    auto ptr = static_cast<MediaH264DecoderVideoToolBox*>(opaque);

    if (ptr->mDecodedFrame) {
        CVPixelBufferRelease(ptr->mDecodedFrame);
        ptr->mDecodedFrame = nullptr;
    }

    if (!image_buffer) {
        H264_DPRINT("%s: output image buffer is null", __func__);
        return;
    }

    ptr->mOutputPts = pts.value;
    ptr->mDecodedFrame = CVPixelBufferRetain(image_buffer);
    // Image is ready to be comsumed
    ptr->copyFrame();
    ptr->mImageReady = true;
    H264_DPRINT("Got decoded frame");
}

// static
CFDictionaryRef MediaH264DecoderVideoToolBox::createOutputBufferAttributes(int width,
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
CMSampleBufferRef MediaH264DecoderVideoToolBox::createSampleBuffer(CMFormatDescriptionRef fmtDesc,
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
OSType MediaH264DecoderVideoToolBox::toNativePixelFormat(PixelFormat pixFmt) {
    switch (pixFmt) {
        case PixelFormat::YUV420P:
            return kCVPixelFormatType_420YpCbCr8Planar;
        case PixelFormat::UYVY422:
            return kCVPixelFormatType_422YpCbCr8;
        case PixelFormat::BGRA8888:
            return kCVPixelFormatType_32BGRA;
        default:
            H264_DPRINT("Unsupported VideoToolbox pixel format");
            return '0000';
    }
}

// static
void* MediaH264DecoderVideoToolBox::getReturnAddress(void* ptr) {
    uint8_t* xptr = (uint8_t*)ptr;
    void* pint = (void*)(xptr + 256);
    return pint;
}

void MediaH264DecoderVideoToolBox::createCMFormatDescription() {
    uint8_t*  parameterSets[2] = {mSPS.data(), mPPS.data()};
    size_t parameterSetSizes[2] = {mSPS.size(), mPPS.size()};

    if (mCmFmtDesc) {
        CFRelease(mCmFmtDesc);
        mCmFmtDesc = nullptr;
    }

    OSStatus status = CMVideoFormatDescriptionCreateFromH264ParameterSets(
                              kCFAllocatorDefault,
                              2, 
                              (const uint8_t *const*)parameterSets, 
                              parameterSetSizes,
                              4, 
                              &mCmFmtDesc);

    if (status == noErr) {
        H264_DPRINT("Created CMFormatDescription from SPS/PPS sets");
    } else {
        H264_DPRINT("Unable to create CMFormatDescription (%d)\n", (int)status);
    }
}

CFDataRef MediaH264DecoderVideoToolBox::createVTDecoderConfig() {
    CFDataRef data = nullptr;
    return data;
}

void MediaH264DecoderVideoToolBox::initH264Context(void* ptr) {
    InitContextParam param{};
    mParser.parseInitContextParams(ptr, param);
    initH264ContextInternal(param.width, param.height, param.outputWidth,
                            param.outputHeight, param.outputPixelFormat);
}

void MediaH264DecoderVideoToolBox::initH264ContextInternal(
        unsigned int width,
        unsigned int height,
        unsigned int outWidth,
        unsigned int outHeight,
        PixelFormat outPixFmt) {
    H264_DPRINT("%s(w=%u h=%u out_w=%u out_h=%u pixfmt=%u)",
                __func__, width, height, outWidth, outHeight, (uint8_t)outPixFmt);
    mWidth = width;
    mHeight = height;
    mOutputWidth = outWidth;
    mOutputHeight = outHeight;
    mOutPixFmt = outPixFmt;
    mOutBufferSize = outWidth * outHeight * 3 / 2;
}

void MediaH264DecoderVideoToolBox::reset(void* ptr) {
    destroyH264Context();
    ResetParam param{};
    mParser.parseResetParams(ptr, param);
    initH264ContextInternal(param.width, param.height, param.outputWidth,
                            param.outputHeight, param.outputPixelFormat);
}

void MediaH264DecoderVideoToolBox::destroyH264Context() {
    H264_DPRINT("%s", __func__);
    if (mDecoderSession) {
        VTDecompressionSessionInvalidate(mDecoderSession);
        CFRelease(mDecoderSession);
        mDecoderSession = nullptr;
    }
    if (mCmFmtDesc) {
        CFRelease(mCmFmtDesc);
        mCmFmtDesc = nullptr;
    }
    if (mDecodedFrame) {
        CVPixelBufferRelease(mDecodedFrame);
        mDecodedFrame = nullptr;
    }
}

static void dumpBytes(const uint8_t* img, size_t szBytes, bool all = false) {
#if MEDIA_H264_DEBUG
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
#endif
}

void MediaH264DecoderVideoToolBox::decodeFrame(void* ptr) {
    DecodeFrameParam param{};
    mParser.parseDecodeFrameParams(ptr, param);

    const uint8_t* frame = param.pData;
    size_t szBytes = param.size;
    uint64_t pts = param.pts;

    decodeFrameInternal(param.pConsumedBytes, param.pDecoderErrorCode, frame, szBytes, pts, 0);
}

void MediaH264DecoderVideoToolBox::oneShotDecode(std::vector<uint8_t> & data, uint64_t pts) {
    //TODO: check if data has more than one Nalu
    decodeFrameInternal(nullptr, nullptr, data.data(), data.size(), pts, 0);
}

void MediaH264DecoderVideoToolBox::decodeFrameInternal(size_t* pRetSzBytes, int32_t* pRetErr, const uint8_t* frame, size_t szBytes, uint64_t pts, size_t consumedSzBytes) {
    // IMPORTANT: There is an assumption that each |frame| we get from the guest are contain
    // complete NALUs. Usually, an H.264 bitstream would be continuously fed to us, but in this case,
    // it seems that the Android frameworks passes us complete NALUs, and may be more than one NALU at
    // a time.
    //
    // Let's only process one NALU per decodeFrame() call because, as soon as we receive a PPS NALU,
    // we need to let the OMX plugin know to reset it's state because we are also recreating our
    // decoder context.
    H264_DPRINT("%s(frame=%p, sz=%zu)", __func__, frame, szBytes);
    Err h264Err = Err::NoErr;

    const uint8_t* currentNalu = H264NaluParser::getNextStartCodeHeader(frame, szBytes);
    if (currentNalu == nullptr) {
        H264_DPRINT("No start code header found in this frame");
        h264Err = Err::NoDecodedFrame;
        // TODO: return the error code and num bytes processed, szBytes.
        if (pRetSzBytes) *pRetSzBytes = szBytes;
        if (pRetErr) *pRetErr = (int32_t)h264Err;
        return;
    }
    const uint8_t* nextNalu = nullptr;
    size_t remaining = szBytes - (currentNalu - frame);

    // Figure out the size of |currentNalu|.
    size_t currentNaluSize = remaining;
    // 3 is the minimum size of the start code header (3 or 4 bytes).
    dumpBytes(currentNalu, currentNaluSize);
    nextNalu = H264NaluParser::getNextStartCodeHeader(currentNalu + 3, remaining - 3);
    if (nextNalu != nullptr) {
        currentNaluSize = nextNalu - currentNalu;
    }

    // |data| is currentNalu, but with the start code header discarded.
    uint8_t* data = nullptr;
    H264NaluType naluType = H264NaluParser::getFrameNaluType(currentNalu, currentNaluSize, &data);
    size_t dataSize = currentNaluSize - (data - currentNalu);
    const std::string naluTypeStr = H264NaluParser::naluTypeToString(naluType);
    H264_DPRINT("Got frame type=%u (%s)", (uint8_t)naluType, naluTypeStr.c_str());

    // We can't do anything until we set up a CMFormatDescription from a set of SPS and PPS NALUs.
    // So just discard the NALU.
    if (naluType != H264NaluType::SPS && naluType != H264NaluType::PPS &&
        mCmFmtDesc == nullptr) {
        H264_DPRINT("CMFormatDescription not set up yet. Need SPS/PPS frames.");
        h264Err = Err::NALUIgnored;
        if (pRetSzBytes) *pRetSzBytes = currentNaluSize;
        if (pRetErr) *pRetErr = (int32_t)h264Err;
        return;
    }

    switch (naluType) {
        case H264NaluType::SPS:
            // We should be getting a PPS frame on the next decodeFrame(). Once we have
            // both sps and pps, we can create/recreate the decoder session.
            // Don't include the start code header when we copy the sps/pps.
            mSPS.assign(data, data + dataSize);
            if (!mIsLoadingFromSnapshot) {
                mSnapshotState = SnapshotState{};
                std::vector<uint8_t> vec;
                vec.assign(currentNalu, currentNalu + currentNaluSize);
                mSnapshotState.saveSps(vec);
            }
            break;
        case H264NaluType::PPS:
            mPPS.assign(data, data + dataSize);
            createCMFormatDescription();
            // TODO: We will need to recreate the decompression session whenever we get a
            // resolution change.
            if (mDecoderSession != nullptr) {
                H264_DPRINT("Decoder session is restarting");
                //h264Err = Err::DecoderRestarted;
            }
            if (!mIsLoadingFromSnapshot) {
                std::vector<uint8_t> vec;
                vec.assign(currentNalu, currentNalu + currentNaluSize);
                mSnapshotState.savePps(vec);
                mSnapshotState.savedPackets.clear();
            }
            recreateDecompressionSession();
            break;
        case H264NaluType::SEI:
//                dumpBytes(nextNalu, remaining, true);
            // In some cases, after the SPS and PPS NALUs are emitted, we'll get a frame that
            // contains both an SEI NALU and a CodedSliceIDR NALU.
            handleSEIFrame(currentNalu, currentNaluSize);
            break;
        case H264NaluType::CodedSliceIDR:
            handleIDRFrame(currentNalu, currentNaluSize, pts);
            if (!mIsLoadingFromSnapshot) {
                H264_DPRINT("disacarding previously saved frames %d", (int)mSnapshotState.savedPackets.size());
                mSnapshotState.savedPackets.clear();
                mSnapshotState.savePacket(currentNalu, currentNaluSize, pts);
            }
            break;
        case H264NaluType::CodedSliceNonIDR:
            handleNonIDRFrame(currentNalu, currentNaluSize, pts);
            if (!mIsLoadingFromSnapshot) {
                mSnapshotState.savePacket(currentNalu, currentNaluSize, pts);
            }
            break;
        default:
            H264_DPRINT("Support for nalu_type=%u not implemented", (uint8_t)naluType);
            break;
    }

    remaining -= currentNaluSize;
    currentNalu = nextNalu;

    // return two things: the error code and the number of bytes we processed.
    if (pRetSzBytes) *pRetSzBytes = currentNaluSize + consumedSzBytes;
    if (pRetErr) *pRetErr = (int32_t)h264Err;

    // disable recursive decoding due to the possibility of session creation failure
    // keep it simple for now
    //if (currentNalu) {
    //    decodeFrameInternal(ptr, currentNalu, remaining, pts, consumedSzBytes + currentNaluSize);
    //}
}

void MediaH264DecoderVideoToolBox::handleIDRFrame(const uint8_t* ptr, size_t szBytes, uint64_t pts) {
    H264_DPRINT("Got IDR frame (sz=%zu)", szBytes);
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
        H264_DPRINT("%s: Failed to create CMSampleBufferRef", __func__);
        return;
    }

    CMSampleBufferSetOutputPresentationTimeStamp(sampleBuf, CMTimeMake(pts, 1));

    OSStatus status;
    status = VTDecompressionSessionDecodeFrame(mDecoderSession,
                                               sampleBuf,
                                               0,       // decodeFlags
                                               NULL,    // sourceFrameRefCon
                                               0);      // infoFlagsOut

    if (status == noErr) {
        // TODO: this call blocks until the frame has been decoded. Perhaps it will be
        // more efficient to signal the guest when the frame is ready to be read instead.
        status = VTDecompressionSessionWaitForAsynchronousFrames(mDecoderSession);
        mIsInFlush = false;
    } else {
        H264_DPRINT("%s: Failed to decompress frame (err=%d)", __func__, status);
    }

    CFRelease(sampleBuf);
    H264_DPRINT("Success decoding IDR frame");
}

void MediaH264DecoderVideoToolBox::handleNonIDRFrame(const uint8_t* ptr, size_t szBytes, uint64_t pts) {
    // Same as handling an IDR frame
    handleIDRFrame(ptr, szBytes, pts);
}

void MediaH264DecoderVideoToolBox::handleSEIFrame(const uint8_t* ptr, size_t szBytes) {
    H264_DPRINT("NOT IMPLEMENTED");
}

void MediaH264DecoderVideoToolBox::flush(void* ptr) {
    H264_DPRINT("%s: NOT IMPLEMENTED", __func__);
    mIsInFlush = true;
}

void MediaH264DecoderVideoToolBox::copyFrame() {
    if (mIsLoadingFromSnapshot) return;

    int imgWidth = CVPixelBufferGetWidth(mDecodedFrame);
    int imgHeight = CVPixelBufferGetHeight(mDecodedFrame);
    int imageSize = CVPixelBufferGetDataSize(mDecodedFrame);
    int stride = CVPixelBufferGetBytesPerRow(mDecodedFrame);

    mOutputWidth = imgWidth;
    mOutputHeight = imgHeight;
    mOutBufferSize = mOutputWidth * mOutputHeight * 3 / 2;

    H264_DPRINT("copying size=%d dimension=[%dx%d] stride=%d", imageSize, imgWidth, imgHeight, stride);

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
            H264_DPRINT("plane=%d data=%p linesize=%d pwidth=%d pheight=%d", i, planeData, linesize, planeWidth, planeHeight);
            // For kCVPixelFormatType_420YpCbCr8Planar, plane 0 is Y, UV planes are 1 and 2
            if (planeWidth != imgWidth && planeWidth != imgWidth / 2) {
                H264_DPRINT("ERROR: Unable to determine YUV420 plane type");
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
            H264_DPRINT("ERROR: Total size of planes not same as guestSz (guestSz=%u, imageSize=%d)", mOutBufferSize, imageSize);
        }
    } else {
        if (imageSize > mOutBufferSize) {
            H264_DPRINT("Buffer size mismatch (guestSz=%u, imageSize=%d). Using guestSz instead.", mOutBufferSize, imageSize);
            imageSize = mOutBufferSize;
        }

        // IMPORTANT: mDecodedFrame must be locked before accessing the contents with CPU
        void* data = CVPixelBufferGetBaseAddress(mDecodedFrame);
        memcpy(dst, data, imageSize);
    }
    CVPixelBufferUnlockBaseAddress(mDecodedFrame, kCVPixelBufferLock_ReadOnly);
}

void MediaH264DecoderVideoToolBox::getImage(void* ptr) {
    // return parameters:
    // 1) either size of the image (> 0) or error code (<= 0).
    // 2) image width
    // 3) image height
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

    if (!mDecodedFrame) {
        H264_DPRINT("%s: frame is null", __func__);
        *retErr = static_cast<int>(Err::NoDecodedFrame);
        return;
    }
    if (!mImageReady) {
        bool hasMoreFrames = false;
        if (mIsInFlush) {
            OSStatus status = noErr;
            status = VTDecompressionSessionWaitForAsynchronousFrames(mDecoderSession);
            if (status == noErr) {
                hasMoreFrames = mImageReady;
                if (hasMoreFrames) {
                    H264_DPRINT("%s: got frame in flush mode", __func__);
                }
            }
        }

        if (!hasMoreFrames) {
            H264_DPRINT("%s: no new frame yet", __func__);
            *retErr = static_cast<int>(Err::NoDecodedFrame);
            return;
        }
    }

    *retWidth = mOutputWidth;
    *retHeight = mOutputHeight;

    if (mParser.version() == 200 && param.hostColorBufferId >= 0) {
        mRenderer.renderToHostColorBuffer(param.hostColorBufferId,
                                          mOutputWidth, mOutputHeight,
                                          mSavedDecodedFrame.data());
    } else {
        memcpy(param.pDecodedFrame, mSavedDecodedFrame.data(), mSavedDecodedFrame.size());;
    }

    *retErr = mSavedDecodedFrame.size();

    *retPts = mOutputPts;

    H264_DPRINT("Copying completed pts %lld", (long long)mOutputPts);
    mImageReady = false;
}

void MediaH264DecoderVideoToolBox::recreateDecompressionSession() {
    if (mCmFmtDesc == nullptr) {
        H264_DPRINT("CMFormatDescription not created. Need sps and pps NALUs.");
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

    mIsInFlush = false;
    mState = DecoderState::BAD_STATE;
    switch (status) {
        case kVTVideoDecoderNotAvailableNowErr:
            H264_DPRINT("VideoToolbox session not available");
            return;
        case kVTVideoDecoderUnsupportedDataFormatErr:
            H264_DPRINT("VideoToolbox does not support this format");
            return;
        case kVTVideoDecoderMalfunctionErr:
            H264_DPRINT("VideoToolbox malfunction");
            return;
        case kVTVideoDecoderBadDataErr:
            H264_DPRINT("VideoToolbox reported invalid data");
            return;
        case 0:
            H264_DPRINT("VideoToolbox session created");
            mState = DecoderState::GOOD_STATE;
            return;
        default:
            H264_DPRINT("Unknown VideoToolbox session creation error %d", status);
            return;
    }
}

void MediaH264DecoderVideoToolBox::save(base::Stream* stream) const {
    stream->putBe32(mParser.version());
    stream->putBe32(mWidth);
    stream->putBe32(mHeight);
    stream->putBe32((int)mOutPixFmt);

    const int hasContext = mDecoderSession != nullptr ? 1 : 0;
    stream->putBe32(hasContext);

    if (mImageReady) {
        mSnapshotState.saveDecodedFrame(
                mSavedDecodedFrame, mOutputWidth, mOutputHeight,
                ColorAspects{}, mOutputPts);
    } else {
        mSnapshotState.savedDecodedFrame.data.clear();
    }
    H264_DPRINT("saving packets now %d",
                (int)(mSnapshotState.savedPackets.size()));
    mSnapshotState.save(stream);
}

bool MediaH264DecoderVideoToolBox::load(base::Stream* stream) {
    mIsLoadingFromSnapshot = true;
    uint32_t version = stream->getBe32();
    mParser = H264PingInfoParser{version};

    mWidth = stream->getBe32();
    mHeight = stream->getBe32();
    mOutPixFmt = (PixelFormat)stream->getBe32();

    const int hasContext = stream->getBe32();
    if (hasContext) {
        initH264ContextInternal(mWidth, mHeight, mWidth, mHeight, mOutPixFmt);
    }
    mSnapshotState.load(stream);
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

    if (mSnapshotState.savedDecodedFrame.data.size() > 0) {
        mSavedDecodedFrame = std::move(mSnapshotState.savedDecodedFrame.data);
        mOutBufferSize = mSnapshotState.savedDecodedFrame.data.size();
        mOutputWidth = mSnapshotState.savedDecodedFrame.width;
        mOutputHeight = mSnapshotState.savedDecodedFrame.height;
        mOutputPts = mSnapshotState.savedDecodedFrame.pts;
        mImageReady = true;
    } else {
        mOutputWidth = mWidth;
        mOutputHeight = mHeight;
        mOutBufferSize = mOutputWidth * mOutputHeight * 3 / 2;
        mImageReady = false;
    }
    mIsLoadingFromSnapshot = false;
    return true;
}

}  // namespace emulation
}  // namespace android
