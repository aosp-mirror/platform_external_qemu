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

#include <VideoToolbox/VideoToolbox.h>

#include <cstdint>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#ifndef kVTVideoDecoderSpecification_RequireHardwareAcceleratedVideoDecoder
#define kVTVideoDecoderSpecification_RequireHardwareAcceleratedVideoDecoder CFSTR("RequireHardwareAcceleratedVideoDecoder")
#endif

#define MEDIA_H264_DEBUG 1

#if MEDIA_H264_DEBUG
#define H264_DPRINT(fmt,...) fprintf(stderr, "h264-dec: %s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define H264_DPRINT(fmt,...)
#endif


namespace android {
namespace emulation {

namespace {

struct SnapshotState {
    // Every component that needs to be saved on exit for snapshots to work should be
    // placed into this structure
    // The decoded video buffer
    mutable CVImageBufferRef decodedFrame = nullptr;
    bool imageReady = false;
    unsigned int outputHeight = 0;
    unsigned int outputWidth = 0;
    mutable MediaH264Decoder::PixelFormat outPixFmt;
    std::vector<uint8_t> sps; // sps NALU
    std::vector<uint8_t> pps; // pps NALU
    mutable CMFormatDescriptionRef cmFmtDesc = nullptr;
    // The output format description. Should be created if we have a valid sps and pps.
    // The VideoToolbox decoder session. Derived from cmFmtDesc creation.
    mutable VTDecompressionSessionRef decoderSession = nullptr;
    // newSps and newPps are new parameter sets that haven't yet been used to create
    // a new format description.
    std::vector<uint8_t> newSps; // new incoming sps
    std::vector<uint8_t> newPps; // new incoming pps
    // Contains the last received IDR frames and any non-IDR frames that follow.
    std::vector<uint8_t> encodedFrames;
};

class MediaH264DecoderImpl : public MediaH264Decoder {
public:
    MediaH264DecoderImpl() = default;
    virtual ~MediaH264DecoderImpl();

    // This is the entry point
    virtual void handlePing(MediaCodecType type, MediaOperation op, void* ptr) override;

    virtual void save(base::Stream* stream) const override;
    virtual bool load(base::Stream* stream) override;
private:
    // TODO(joshuaduong) Dead code ATM; was intended to be used to snapshot the last decoded frame,
    // but it seems that the non-IDR frames may reference previous non-IDR frames. So simply having
    // a key frame before it may not work, although I haven't verified that yet, since the decoder
    // is having trouble decoding this encoded frame using the SPS/PPS sets given to us by the guest.
    // Leaving this code here as it may be useful for someone else.
    static void videoToolboxCompressCallback(void *ctx,
                                             void *sourceFrameCtx,
                                             OSStatus status,
                                             VTEncodeInfoFlags flags,
                                             CMSampleBufferRef sample_buffer);
    // Saves the encoded frames to the snapshot
    void saveEncodedFrames(base::Stream* stream) const;
    // Loads the encoded frames from the snapshot
    void loadEncodedFrames(base::Stream* stream);

    // Passes the Sequence Parameter Set (SPS) and Picture Parameter Set (PPS) to the
    // videotoolbox decoder
    CFDataRef createVTDecoderConfig();
    // Callback passed to the VTDecompressionSession
    static void videoToolboxDecompressCallback(void* opaque,
                                               void* sourceFrameRefCon,
                                               OSStatus status,
                                               VTDecodeInfoFlags flags,
                                               CVImageBufferRef image_buffer,
                                               CMTime pts,
                                               CMTime duration);
    static CFDictionaryRef createOutputBufferAttributes(int width,
                                                        int height,
                                                        OSType pix_fmt);
    static CMSampleBufferRef createSampleBuffer(CMFormatDescriptionRef fmtDesc,
                                                void* buffer,
                                                size_t sz);
    static OSType toNativePixelFormat(PixelFormat fmt);

    // We should move these shared memory calls elsewhere, as vpx decoder is also using the same/similar
    // functions
    static void* getReturnAddress(void* ptr);
    static uint8_t* getDst(void* ptr);

    virtual void initH264Context(unsigned int width,
                                 unsigned int height,
                                 unsigned int outWidth,
                                 unsigned int outHeight,
                                 PixelFormat pixFmt) override;
    virtual void destroyH264Context() override;
    virtual void decodeFrame(void* ptr, const uint8_t* frame, size_t szBytes) override;
    virtual void flush(void* ptr) override;
    virtual void getImage(void* ptr) override;
    
    void handleIDRFrame(const uint8_t* ptr, size_t szBytes);
    void handleNonIDRFrame(const uint8_t* ptr, size_t szBytes);
    void handleSEIFrame(const uint8_t* ptr, size_t szBytes);

    void createCMFormatDescription();
    void recreateDecompressionSession();
    
    static constexpr int kBPP = 2; // YUV420 is 2 bytes per pixel
    // The NAL units outputted by the VideoToolbox always is prefixed with a 4-byte
    // value indicating the payload size.
    static constexpr int kPayloadSizeLen = 4;
    std::vector<uint8_t> mEncodedFrame;
    SnapshotState mState;
}; // MediaH264DecoderImpl

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

MediaH264DecoderImpl::~MediaH264DecoderImpl() {
    destroyH264Context();
}

// static
void MediaH264DecoderImpl::videoToolboxDecompressCallback(void* opaque,
                                                          void* sourceFrameRefCon,
                                                          OSStatus status,
                                                          VTDecodeInfoFlags flags,
                                                          CVImageBufferRef image_buffer,
                                                          CMTime pts,
                                                          CMTime duration) {
    auto ptr = static_cast<MediaH264DecoderImpl*>(opaque);

    if (ptr->mState.decodedFrame) {
        CVPixelBufferRelease(ptr->mState.decodedFrame);
        ptr->mState.decodedFrame = nullptr;
    }

    if (!image_buffer || status) {
        H264_DPRINT("output image buffer is null (%d)", status);
        return;
    }

    ptr->mState.decodedFrame = CVPixelBufferRetain(image_buffer);
    // Image is ready to be comsumed
    ptr->mState.imageReady = true;
    H264_DPRINT("Got decoded frame");
}

// static
CFDictionaryRef MediaH264DecoderImpl::createOutputBufferAttributes(int width,
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
CMSampleBufferRef MediaH264DecoderImpl::createSampleBuffer(CMFormatDescriptionRef fmtDesc,
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
OSType MediaH264DecoderImpl::toNativePixelFormat(PixelFormat pixFmt) {
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
void* MediaH264DecoderImpl::getReturnAddress(void* ptr) {
    uint8_t* xptr = (uint8_t*)ptr;
    void* pint = (void*)(xptr + 256);
    return pint;
}

// static
uint8_t* MediaH264DecoderImpl::getDst(void* ptr) {
    // Guest will pass us the offset from the start address for where to write the image data.
    uint8_t* xptr = (uint8_t*)ptr;
    uint64_t offset = *(uint64_t*)(xptr);
    H264_DPRINT("start=%p dst=%p offset=%llu", xptr, xptr + offset, offset);
    return (uint8_t*)ptr + offset;
}

void MediaH264DecoderImpl::createCMFormatDescription() {
    uint8_t*  parameterSets[2] = {mState.sps.data(), mState.pps.data()};
    size_t parameterSetSizes[2] = {mState.sps.size(), mState.pps.size()};

    if (mState.cmFmtDesc) {
        CFRelease(mState.cmFmtDesc);
        mState.cmFmtDesc = nullptr;
    }

    OSStatus status = CMVideoFormatDescriptionCreateFromH264ParameterSets(
                              kCFAllocatorDefault,
                              2, 
                              (const uint8_t *const*)parameterSets, 
                              parameterSetSizes,
                              4, 
                              &mState.cmFmtDesc);

    if (status == noErr) {
        H264_DPRINT("Created CMFormatDescription from SPS/PPS sets");
    } else {
        H264_DPRINT("Unable to create CMFormatDescription (%d)\n", (int)status);
    }
}

CFDataRef MediaH264DecoderImpl::createVTDecoderConfig() {
    CFDataRef data = nullptr;
    return data;
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

// static
void MediaH264DecoderImpl::videoToolboxCompressCallback(
    void *ctx,
    void *sourceFrameCtx,
    OSStatus status,
    VTEncodeInfoFlags flags,
    CMSampleBufferRef sample_buffer)
{
    H264_DPRINT("%s", __func__);
    auto ptr = static_cast<MediaH264DecoderImpl*>(ctx);

    if (status || !sample_buffer) {
        H264_DPRINT("Error encoding frame: %d", (int)status);
        return;
    }

    H264_DPRINT("Got encoded frame");
    // The encoded data is a sequence of NAL units (for the first frame, probably
    // just SEI data and then the IDR frame). We can just ignore everything except for the
    // IDR data since we already have the SPS, PPS, and SEI data.
    //
    // Also, the NAL units are in the following format:
    // ==============================================================
    // | size of payload (4-bytes) | NAL unit info (1 byte) | payload
    // ==============================================================
    uint8_t payloadSizeBytes[4];
    uint8_t naluType;
    CMBlockBufferRef block = CMSampleBufferGetDataBuffer(sample_buffer);
    size_t src_size = CMSampleBufferGetTotalSampleSize(sample_buffer);
    int64_t remainingSize = src_size;
    size_t offset = 0;

    H264_DPRINT("Encoded SampleBuffer size=%zu", src_size);
    while (remainingSize > 0) {
        int status = CMBlockBufferCopyDataBytes(block,
                                                offset,
                                                kPayloadSizeLen,
                                                payloadSizeBytes);
        if (status) {
            H264_DPRINT("Unable to read NAL unit length (%d)", status);
            return;
        }

        uint32_t payloadSize = 0;
        for (int i = 0; i < kPayloadSizeLen; ++i) {
            payloadSize <<= 8;
            payloadSize += payloadSizeBytes[i];
        }

        status = CMBlockBufferCopyDataBytes(block,
                                            offset + kPayloadSizeLen,
                                            1,
                                            &naluType);

        naluType &= 0x1F;
        if (naluType != (uint8_t)H264NaluType::CodedSliceIDR) {
            remainingSize -= kPayloadSizeLen + payloadSize;
            offset += kPayloadSizeLen + payloadSize;
            continue;
        }

        ptr->mEncodedFrame.resize(kPayloadSizeLen + payloadSize);
        H264_DPRINT("PayloadSize=%u bytes", payloadSize);

        // Keep the NAL unit as-is; with the four-byte payload size and the payload.
        // it makes it convenient to load from the snapshot later, at which we can replace the
        // payload size with the four-byte start code header.
        status = CMBlockBufferCopyDataBytes(block,
                                            offset,
                                            kPayloadSizeLen + payloadSize,
                                            ptr->mEncodedFrame.data());
        if (status) {
            H264_DPRINT("Unable to copy data bytes (%d)", status);
            return;
        }

        dumpBytes(ptr->mEncodedFrame.data(), 256 < ptr->mEncodedFrame.size() ? 256 : ptr->mEncodedFrame.size(), true);
        break;
    }
}

void MediaH264DecoderImpl::saveEncodedFrames(base::Stream* stream) const {
    if (mState.encodedFrames.size() == 0) {
        H264_DPRINT("No encoded frames to save");
        stream->putBe64(0);
        return;
    }

    H264_DPRINT("Saving the H.264 encoded frames (size=%u)", mState.encodedFrames.size());
    stream->putBe64(mState.encodedFrames.size());
    stream->write(mState.encodedFrames.data(), mState.encodedFrames.size());
}

void MediaH264DecoderImpl::loadEncodedFrames(base::Stream* stream) {
    uint64_t sz = stream->getBe64();
    if (sz == 0) {
        H264_DPRINT("No encoded frames to load from snapshot.");
        return;
    }

    mState.encodedFrames.resize(sz);
    H264_DPRINT("Reading encodedFrames data (size=%llu)", sz);
    stream->read(mState.encodedFrames.data(), sz);

    auto remaining = mState.encodedFrames.size();

    if (remaining < 4) {
        H264_DPRINT("Not enough data for an encoded frame.");
        return;
    }

    const uint8_t* currentNalu = getNextStartCodeHeader(mState.encodedFrames.data(), remaining);
    if (currentNalu == nullptr) {
        return;
    }

    do {
        const uint8_t* nextNalu = getNextStartCodeHeader(currentNalu + 3, remaining - 3);
        size_t currentNaluSize;
        if (nextNalu == nullptr) {
            currentNaluSize = remaining;
        } else {
            currentNaluSize = nextNalu - currentNalu;
        }

        handleIDRFrame(currentNalu, currentNaluSize);
        currentNalu = nextNalu;
        remaining -= currentNaluSize;
    } while (currentNalu != nullptr);
    H264_DPRINT("Finished loading H.264 encoded frames from snapshot");
}

void MediaH264DecoderImpl::save(base::Stream* stream) const {
    // We need to save the SPS, PPS, and if we have a decoded frame, to generate an IDR frame
    // from that, otherwise we cannot decode the next non-IDR frame we get.
    stream->putByte(mState.imageReady ? 1 : 0);
    stream->putByte((uint8_t)mState.outPixFmt);
    stream->putBe32(mState.outputHeight);
    stream->putBe32(mState.outputWidth);

    auto paramSets = {&mState.sps, &mState.pps, &mState.newSps, &mState.newPps};
    for (int i = 0; i < 4; ++i) {
        stream->putBe64(paramSets.begin()[i]->size());
        if (paramSets.begin()[i]->size() > 0) {
            stream->write(paramSets.begin()[i]->data(), paramSets.begin()[i]->size());
        }
    }

    saveEncodedFrames(stream);
    H264_DPRINT("Saved H264 decoder snapshot");
}

bool MediaH264DecoderImpl::load(base::Stream* stream) {
    mState.imageReady = stream->getByte() == 1 ? true : false;
    mState.outPixFmt = (PixelFormat)stream->getByte();
    mState.outputHeight = stream->getBe32();
    mState.outputWidth = stream->getBe32();

    auto paramSets = {&mState.sps, &mState.pps, &mState.newSps, &mState.newPps};
    for (int i = 0; i < 4; ++i) {
        size_t sz = stream->getBe64();
        if (sz > 0) {
            paramSets.begin()[i]->resize(sz);
            stream->read(paramSets.begin()[i]->data(), sz);
        }
    }

    if (mState.sps.size() > 0 && mState.pps.size() > 0) {
        H264_DPRINT("Recreating H264 decoder session from snapshot");
        createCMFormatDescription();
        recreateDecompressionSession();
        loadEncodedFrames(stream);
    }

    H264_DPRINT("Done loading H264 codec snapshot");
    return true;
}

void MediaH264DecoderImpl::initH264Context(unsigned int width,
                                           unsigned int height,
                                           unsigned int outWidth,
                                           unsigned int outHeight,
                                           PixelFormat outPixFmt) {
    H264_DPRINT("%s(w=%u h=%u out_w=%u out_h=%u pixfmt=%u)",
                __func__, width, height, outWidth, outHeight, (uint8_t)outPixFmt);
    mState.outputWidth = outWidth;
    mState.outputHeight = outHeight;
    mState.outPixFmt = outPixFmt;
}

void MediaH264DecoderImpl::destroyH264Context() {
    H264_DPRINT("%s", __func__);
    if (mState.decoderSession) {
        VTDecompressionSessionInvalidate(mState.decoderSession);
        CFRelease(mState.decoderSession);
        mState.decoderSession = nullptr;
    }
    if (mState.cmFmtDesc) {
        CFRelease(mState.cmFmtDesc);
        mState.cmFmtDesc = nullptr;
    }
    if (mState.decodedFrame) {
        CVPixelBufferRelease(mState.decodedFrame);
        mState.decodedFrame = nullptr;
    }
}

void MediaH264DecoderImpl::decodeFrame(void* ptr, const uint8_t* frame, size_t szBytes) {
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
    // TODO: move this somewhere else
    // First return parameter is the number of bytes processed,
    // Second return parameter is the error code
    uint8_t* retptr = (uint8_t*)getReturnAddress(ptr);
    uint64_t* retSzBytes = (uint64_t*)retptr;
    int32_t* retErr = (int32_t*)(retptr + 8);

    const uint8_t* currentNalu = getNextStartCodeHeader(frame, szBytes);
    if (currentNalu == nullptr) {
        H264_DPRINT("No start code header found in this frame");
        h264Err = Err::NoDecodedFrame;
        // TODO: return the error code and num bytes processed, szBytes.
        *retSzBytes = szBytes;
        *retErr = (int32_t)h264Err;
        return;
    }
    const uint8_t* nextNalu = nullptr;
    size_t remaining = szBytes - (currentNalu - frame);

    // Figure out the size of |currentNalu|.
    size_t currentNaluSize = remaining;
    // 3 is the minimum size of the start code header (3 or 4 bytes).
    dumpBytes(currentNalu, currentNaluSize);
    nextNalu = getNextStartCodeHeader(currentNalu + 3, remaining - 3);
    if (nextNalu != nullptr) {
        currentNaluSize = nextNalu - currentNalu;
    }

    // |data| is currentNalu, but with the start code header discarded.
    uint8_t* data = nullptr;
    H264NaluType naluType = getFrameNaluType(currentNalu, currentNaluSize, &data);
    size_t dataSize = currentNaluSize - (data - currentNalu);
    const std::string naluTypeStr = naluTypeToString(naluType);
    H264_DPRINT("Got frame type=%u (%s)", (uint8_t)naluType, naluTypeStr.c_str());

    // We can't do anything until we set up a CMFormatDescription from a set of SPS and PPS NALUs.
    // So just discard the NALU.
    if (naluType != H264NaluType::SPS && naluType != H264NaluType::PPS &&
        mState.cmFmtDesc == nullptr) {
        H264_DPRINT("CMFormatDescription not set up yet. Need SPS/PPS frames.");
        h264Err = Err::NALUIgnored;
        // TODO: return the error code and num bytes processed, szBytes.
        *retSzBytes = currentNaluSize;
        *retErr = (int32_t)h264Err;
        return;
    }

    switch (naluType) {
        case H264NaluType::SPS:
            // We should be getting a PPS frame on the next decodeFrame(). Once we have
            // both sps and pps, we can create/recreate the decoder session.
            // Don't include the start code header when we copy the sps/pps.
            mState.newSps.assign(data, data + dataSize);
            break;
        case H264NaluType::PPS:
            mState.newPps.assign(data, data + dataSize);
            mState.sps = std::move(mState.newSps);
            mState.newSps.clear();
            mState.pps = std::move(mState.newPps);
            mState.newPps.clear();
            createCMFormatDescription();
            // TODO: We will need to recreate the decompression session whenever we get a
            // resolution change.
            if (mState.decoderSession != nullptr) {
                H264_DPRINT("Decoder session is restarting");
                h264Err = Err::DecoderRestarted;
            }
            recreateDecompressionSession();

            // Once we get a new set of SPS/PPS, we can throw away the set of IDR/non-IDR
            // frames we were saving, because a new IDR frame is about to come in.
            mState.encodedFrames.clear();
            break;
        case H264NaluType::SEI:
//                dumpBytes(nextNalu, remaining, true);
            // In some cases, after the SPS and PPS NALUs are emitted, we'll get a frame that
            // contains both an SEI NALU and a CodedSliceIDR NALU.
            handleSEIFrame(currentNalu, currentNaluSize);
            break;
        case H264NaluType::CodedSliceIDR:
            handleIDRFrame(currentNalu, currentNaluSize);
            mState.encodedFrames.insert(mState.encodedFrames.end(), currentNalu, currentNalu + currentNaluSize);
            break;
        case H264NaluType::CodedSliceNonIDR:
            handleNonIDRFrame(currentNalu, currentNaluSize);
            mState.encodedFrames.insert(mState.encodedFrames.end(), currentNalu, currentNalu + currentNaluSize);
            break;
        default:
            H264_DPRINT("Support for nalu_type=%u not implemented", (uint8_t)naluType);
            break;
    }

    remaining -= currentNaluSize;
    currentNalu = nextNalu;

    // return two things: the error code and the number of bytes we processed.
    *retSzBytes = currentNaluSize;
    *retErr = (int32_t)h264Err;
}

void MediaH264DecoderImpl::handleIDRFrame(const uint8_t* ptr, size_t szBytes) {
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
    sampleBuf = createSampleBuffer(mState.cmFmtDesc, (void*)idr.get(), dataSz + 4);
    if (!sampleBuf) {
        H264_DPRINT("%s: Failed to create CMSampleBufferRef", __func__);
        return;
    }

    OSStatus status;
    status = VTDecompressionSessionDecodeFrame(mState.decoderSession,
                                               sampleBuf,
                                               0,       // decodeFlags
                                               NULL,    // sourceFrameRefCon
                                               0);      // infoFlagsOut

    if (status == noErr) {
        // TODO: this call blocks until the frame has been decoded. Perhaps it will be
        // more efficient to signal the guest when the frame is ready to be read instead.
        status = VTDecompressionSessionWaitForAsynchronousFrames(mState.decoderSession);
    } else {
        H264_DPRINT("%s: Failed to decompress frame (err=%d)", __func__, status);
    }

    CFRelease(sampleBuf);
    H264_DPRINT("Success decoding IDR frame");
}

void MediaH264DecoderImpl::handleNonIDRFrame(const uint8_t* ptr, size_t szBytes) {
    // Same as handling an IDR frame
    handleIDRFrame(ptr, szBytes);
}

void MediaH264DecoderImpl::handleSEIFrame(const uint8_t* ptr, size_t szBytes) {
    H264_DPRINT("NOT IMPLEMENTED");
}

void MediaH264DecoderImpl::flush(void* ptr) {
    H264_DPRINT("%s: NOT IMPLEMENTED", __func__);
}

void MediaH264DecoderImpl::getImage(void* ptr) {
    // return parameters:
    // 1) either size of the image (> 0) or error code (<= 0).
    // 2) image width
    // 3) image height
    uint8_t* retptr = (uint8_t*)getReturnAddress(ptr);
    int* retErr = (int*)(retptr);
    uint32_t* retWidth = (uint32_t*)(retptr + 8);
    uint32_t* retHeight = (uint32_t*)(retptr + 16);

    if (!mState.decodedFrame) {
        H264_DPRINT("%s: frame is null", __func__);
        *retErr = static_cast<int>(Err::NoDecodedFrame);
        return;
    }
    if (!mState.imageReady) {
        H264_DPRINT("%s: no new frame yet", __func__);
        *retErr = static_cast<int>(Err::NoDecodedFrame);
        return;
    }

    int imgWidth = CVPixelBufferGetWidth(mState.decodedFrame);
    int imgHeight = CVPixelBufferGetHeight(mState.decodedFrame);
    int imageSize = CVPixelBufferGetDataSize(mState.decodedFrame);
    int stride = CVPixelBufferGetBytesPerRow(mState.decodedFrame);

    *retWidth = imgWidth;
    *retHeight = imgHeight;

    H264_DPRINT("copying size=%d dimension=[%dx%d] stride=%d", imageSize, imgWidth, imgHeight, stride);

    // Copies the image data to the guest.
    uint8_t* dst =  getDst(ptr);

    // The calculated size of the outHeader buffer size allocated in the guest.
    // It should be sizeY + (sizeUV * 2), where:
    //  sizeY = mState.outputWidth * mState.outputHeight,
    //  sizeUV = sizeY / 4
    // It is equivalent to mState.outputWidth * mState.outputHeight * 3 / 2
    unsigned int outBufferSize = mState.outputWidth * mState.outputHeight * 3 / 2;

    CVPixelBufferLockBaseAddress(mState.decodedFrame, kCVPixelBufferLock_ReadOnly);
    if (CVPixelBufferIsPlanar(mState.decodedFrame)) {
        imageSize = 0; // add up the size from the planes
        int planes = CVPixelBufferGetPlaneCount(mState.decodedFrame);
        for (int i = 0; i < planes; ++i) {
            void* planeData = CVPixelBufferGetBaseAddressOfPlane(mState.decodedFrame, i);
            int linesize = CVPixelBufferGetBytesPerRowOfPlane(mState.decodedFrame, i);
            int planeWidth = CVPixelBufferGetWidthOfPlane(mState.decodedFrame, i);
            int planeHeight = CVPixelBufferGetHeightOfPlane(mState.decodedFrame, i);
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
        if (imageSize != outBufferSize) {
            H264_DPRINT("ERROR: Total size of planes not same as guestSz (guestSz=%u, imageSize=%d)", outBufferSize, imageSize);
        }
    } else {
        if (imageSize > outBufferSize) {
            H264_DPRINT("Buffer size mismatch (guestSz=%u, imageSize=%d). Using guestSz instead.", outBufferSize, imageSize);
            imageSize = outBufferSize;
        }

        // IMPORTANT: mState.decodedFrame must be locked before accessing the contents with CPU
        void* data = CVPixelBufferGetBaseAddress(mState.decodedFrame);
        memcpy(dst, data, imageSize);
    }
    CVPixelBufferUnlockBaseAddress(mState.decodedFrame, kCVPixelBufferLock_ReadOnly);

    *retErr = imageSize;


    H264_DPRINT("Copying completed");
    mState.imageReady = false;
}

void MediaH264DecoderImpl::recreateDecompressionSession() {
    if (mState.cmFmtDesc == nullptr) {
        H264_DPRINT("CMFormatDescription not created. Need sps and pps NALUs.");
        return;
    }

    // Create a new VideoToolbox decoder session if one already exists
    if (mState.decoderSession != nullptr) {
        // TODO: Once we implement async frame readback, we'll need to flush all of the frames here and
        // store them somewhere for the guest to read later.
        VTDecompressionSessionInvalidate(mState.decoderSession);
        CFRelease(mState.decoderSession);
        mState.decoderSession = nullptr;
        if (mState.decodedFrame) {
            CVPixelBufferRelease(mState.decodedFrame);
            mState.decodedFrame = nullptr;
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

    CFDictionaryRef bufAttr = createOutputBufferAttributes(mState.outputWidth,
                                                           mState.outputHeight,
                                                           toNativePixelFormat(mState.outPixFmt));

    VTDecompressionOutputCallbackRecord decoderCb;
    decoderCb.decompressionOutputCallback = videoToolboxDecompressCallback;
    decoderCb.decompressionOutputRefCon = this;

    OSStatus status;
    status = VTDecompressionSessionCreate(NULL,              // allocator
                                          mState.cmFmtDesc,        // videoFormatDescription
                                          decoder_spec,      // videoDecoderSpecification
                                          bufAttr,           // destinationImageBufferAttributes
                                          &decoderCb,        // outputCallback
                                          &mState.decoderSession); // decompressionSessionOut

    if (decoder_spec) {
        CFRelease(decoder_spec);
    }
    if (bufAttr) {
        CFRelease(bufAttr);
    }

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
            return;
        default:
            H264_DPRINT("Unknown VideoToolbox session creation error %d", status);
            return;
    }
}

};  // namespace

// static
MediaH264Decoder* MediaH264Decoder::create() {
    return new MediaH264DecoderImpl();
}

}  // namespace emulation
}  // namespace android
