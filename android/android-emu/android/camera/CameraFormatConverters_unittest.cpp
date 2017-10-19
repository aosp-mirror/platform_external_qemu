// Copyright (C) 2017 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/camera/camera-format-converters.h"

#include "android/base/ArraySize.h"
#include "android/base/testing/TestSystem.h"
#include "android/base/testing/TestTempDir.h"

#include <gtest/gtest.h>

#include <vector>
#include <assert.h>

using android::base::TestSystem;
using android::base::System;

namespace std {
void PrintTo(const vector<uint8_t>& vec, ostream* os) {
    ios::fmtflags origFlags(os->flags());

    *os << '{';
    size_t count = 0;
    for (vector<uint8_t>::const_iterator it = vec.begin(); it != vec.end();
         ++it, ++count) {
        if (count > 0) {
            *os << ", ";
        }
        if ((count % 16) == 0) {
            *os << "\n";
        }

        if (*it == 0) {
            *os << "    ";
        } else {
            *os << "0x" << std::hex << std::uppercase << std::setw(2)
                << std::setfill('0') << int(*it) << std::dec;
        }
    }

    *os << '}';

    os->flags(origFlags);
}
}

// A list of all of the source formats that convert_frame_slow understands,
// taken from camera-format-converters.c's _PIXFormats.
static constexpr uint32_t kSupportedSourceFormats[] = {
        // RGB formats.
        V4L2_PIX_FMT_ARGB32, V4L2_PIX_FMT_RGB32, V4L2_PIX_FMT_BGR32,
        V4L2_PIX_FMT_RGB565, V4L2_PIX_FMT_RGB24, V4L2_PIX_FMT_BGR24,

        // YUV 4:2:0 formats.
        V4L2_PIX_FMT_YVU420, V4L2_PIX_FMT_YUV420, V4L2_PIX_FMT_NV12,
        V4L2_PIX_FMT_NV21,

        // YUV 4:2:2 formats.
        V4L2_PIX_FMT_YUYV, V4L2_PIX_FMT_YYUV, V4L2_PIX_FMT_YVYU,
        V4L2_PIX_FMT_UYVY, V4L2_PIX_FMT_VYUY, V4L2_PIX_FMT_YYVU,
        // Aliases for V4L2_PIX_FMT_YUYV.
        V4L2_PIX_FMT_YUY2, V4L2_PIX_FMT_YUNV, V4L2_PIX_FMT_V422,

        // Exclude bayer formats, they are not available in the fast-path so
        // they don't need comparative testing.
};

// A list of supported output formats, taken from camera-service.c's
// calculate_frame_size.
static constexpr uint32_t kSupportedDestinationFormats[] = {
        V4L2_PIX_FMT_YUV420, V4L2_PIX_FMT_YVU420, V4L2_PIX_FMT_NV12,
        V4L2_PIX_FMT_NV21,   V4L2_PIX_FMT_RGB32,  V4L2_PIX_FMT_RGB24,
};

static size_t bufferSize(uint32_t format, int width, int height) {
    size_t size = 0;
    EXPECT_TRUE(calculate_framebuffer_size(format, width, height, &size));
    return size;
}

static std::string fourccToString(uint32_t fourcc) {
    return std::string(reinterpret_cast<const char*>(&fourcc),
                       sizeof(uint32_t));
}

// Generate a framebuffer filled with the given color (in rgb) and converted
// to the target colorspace.
static std::vector<uint8_t> generateFramebuffer(uint32_t format,
                                                int width,
                                                int height,
                                                uint8_t a,
                                                uint8_t r,
                                                uint8_t g,
                                                uint8_t b) {
    uint32_t argbColor;
    uint8_t* rgbPtr = reinterpret_cast<uint8_t*>(&argbColor);
    rgbPtr[0] = a;
    rgbPtr[1] = r;
    rgbPtr[2] = g;
    rgbPtr[3] = b;

    const size_t srcSize = bufferSize(V4L2_PIX_FMT_ARGB32, width, height);
    const size_t destSize = bufferSize(format, width, height);
    std::vector<uint8_t> src(srcSize);
    std::vector<uint8_t> dest(destSize);

    // Fill src with the argbColor
    uint32_t* src32 = reinterpret_cast<uint32_t*>(src.data());
    for (size_t i = 0; i < srcSize / 4; ++i) {
        src32[i] = argbColor;
    }

    ClientFrameBuffer framebuffer = {};
    framebuffer.pixel_format = format;
    framebuffer.framebuffer = dest.data();

    EXPECT_EQ(0, convert_frame_slow(src.data(), V4L2_PIX_FMT_ARGB32, src.size(),
                                    width, height, &framebuffer, 1, 1.0f, 1.0f,
                                    1.0f, 1.0f))
            << "convert_frame_slow, source="
            << fourccToString(V4L2_PIX_FMT_ARGB32)
            << " dest=" << fourccToString(format);

    return dest;
}

static bool vectorNearMatch(const std::vector<uint8_t>& src,
                            const std::vector<uint8_t>& dest) {
    if (src.size() != dest.size()) {
        return false;
    }

    bool match = true;

    for (size_t i = 0; match && i < src.size(); ++i) {
        match = (abs(static_cast<int>(src[i]) - dest[i]) <= 2);
    }

    if (!match) {
        // Fall back to standard EXPECT_EQ for better error output.
        EXPECT_EQ(src, dest);
    }

    return match;
}

class ConvertWithSize : public testing::TestWithParam<int> {};
class OddDimensions : public testing::TestWithParam<int> {};

// Validate identity conversions.
TEST_P(ConvertWithSize, SlowPathIdentity) {
    const int widthOrHeight = GetParam();

    for (uint32_t format : kSupportedDestinationFormats) {
        std::vector<uint8_t> src = generateFramebuffer(
                format, widthOrHeight, widthOrHeight, 0xFF, 0x00, 0x66, 0xCC);
        const size_t destSize =
                bufferSize(format, widthOrHeight, widthOrHeight);
        std::vector<uint8_t> dest(destSize);

        ClientFrameBuffer framebuffer = {};
        framebuffer.pixel_format = format;
        framebuffer.framebuffer = dest.data();

        EXPECT_EQ(0,
                  convert_frame_slow(src.data(), format, src.size(),
                                     widthOrHeight, widthOrHeight, &framebuffer,
                                     1, 1.0f, 1.0f, 1.0f, 1.0f))
                << "convert_frame_slow, source=" << fourccToString(format)
                << " dest=" << fourccToString(format);

        EXPECT_TRUE(vectorNearMatch(src, dest)) << "format="
                                                << fourccToString(format);
    }
}

TEST_P(ConvertWithSize, FastPathIdentity) {
    const int widthOrHeight = GetParam();
    std::vector<uint8_t> stagingFramebuffer(
            bufferSize(V4L2_PIX_FMT_YUV420, widthOrHeight, widthOrHeight));

    for (uint32_t format : kSupportedDestinationFormats) {
        std::vector<uint8_t> src = generateFramebuffer(
                format, widthOrHeight, widthOrHeight, 0xFF, 0x00, 0x66, 0xCC);
        const size_t destSize =
                bufferSize(format, widthOrHeight, widthOrHeight);
        std::vector<uint8_t> dest(destSize);

        ClientFrameBuffer framebuffer = {};
        framebuffer.pixel_format = format;
        framebuffer.framebuffer = dest.data();

        ClientFrame resultFrame = {};
        resultFrame.framebuffers = &framebuffer;
        resultFrame.framebuffers_count = 1;
        resultFrame.staging_framebuffer = stagingFramebuffer.data();
        resultFrame.staging_framebuffer_size = stagingFramebuffer.size();

        EXPECT_EQ(0, convert_frame_fast(src.data(), format, src.size(),
                                        widthOrHeight, widthOrHeight,
                                        &resultFrame, 1.0f))
                << "convert_frame_fast, source=" << fourccToString(format)
                << " dest=" << fourccToString(format);

        EXPECT_TRUE(vectorNearMatch(src, dest)) << "format="
                                                << fourccToString(format);
    }
}

// Validate conversions from all source formats to all dest formats.
TEST_P(ConvertWithSize, SourceToDest) {
    const int widthOrHeight = GetParam();
    const size_t stagingSize =
            bufferSize(V4L2_PIX_FMT_YUV420, widthOrHeight, widthOrHeight);
    std::vector<uint8_t> stagingFramebuffer(stagingSize);

    for (uint32_t src_format : kSupportedSourceFormats) {
        std::vector<uint8_t> src =
                generateFramebuffer(src_format, widthOrHeight, widthOrHeight,
                                    0xFF, 0x00, 0x66, 0xCC);

        for (uint32_t dest_format : kSupportedDestinationFormats) {
            const size_t destSize =
                    bufferSize(dest_format, widthOrHeight, widthOrHeight);
            std::vector<uint8_t> dest(destSize);

            ClientFrameBuffer framebuffer = {};
            framebuffer.pixel_format = dest_format;
            framebuffer.framebuffer = dest.data();

            ClientFrame resultFrame = {};
            resultFrame.framebuffers = &framebuffer;
            resultFrame.framebuffers_count = 1;
            resultFrame.staging_framebuffer = stagingFramebuffer.data();
            resultFrame.staging_framebuffer_size = stagingFramebuffer.size();

            EXPECT_EQ(0, convert_frame(src.data(), src_format, src.size(),
                                       widthOrHeight, widthOrHeight,
                                       &resultFrame, 1.0f, 1.0f, 1.0f, 1.0f))
                    << "convert_frame, source=" << fourccToString(src_format)
                    << " dest=" << fourccToString(dest_format);

            std::vector<uint8_t> destBaseline(destSize);
            ClientFrameBuffer baselineFramebuffer = {};
            baselineFramebuffer.pixel_format = dest_format;
            baselineFramebuffer.framebuffer = destBaseline.data();

            EXPECT_EQ(0, convert_frame_slow(src.data(), src_format, src.size(),
                                            widthOrHeight, widthOrHeight,
                                            &baselineFramebuffer, 1, 1.0f, 1.0f,
                                            1.0f, 1.0f))
                    << "convert_frame_slow, source="
                    << fourccToString(src_format)
                    << " dest=" << fourccToString(dest_format);

            EXPECT_TRUE(vectorNearMatch(destBaseline, dest))
                    << "src_format=" << fourccToString(src_format)
                    << " dest_format=" << fourccToString(dest_format);
        }
    }
}

// The fast-path and slow-path handle boundary pixels for odd dimensions
// differently, so the two methods can't be directly compared.  Validate that
// conversion succeeds and doesn't crash.
TEST_P(OddDimensions, SourceToDest) {
    const int widthOrHeight = GetParam();
    const size_t stagingSize =
            bufferSize(V4L2_PIX_FMT_YUV420, widthOrHeight, widthOrHeight);
    std::vector<uint8_t> stagingFramebuffer(stagingSize);

    for (uint32_t src_format : kSupportedSourceFormats) {
        std::vector<uint8_t> src =
                generateFramebuffer(src_format, widthOrHeight, widthOrHeight,
                                    0xFF, 0x00, 0x66, 0xCC);

        for (uint32_t dest_format : kSupportedDestinationFormats) {
            const size_t destSize =
                    bufferSize(dest_format, widthOrHeight, widthOrHeight);
            std::vector<uint8_t> dest(destSize);

            ClientFrameBuffer framebuffer = {};
            framebuffer.pixel_format = dest_format;
            framebuffer.framebuffer = dest.data();

            ClientFrame resultFrame = {};
            resultFrame.framebuffers = &framebuffer;
            resultFrame.framebuffers_count = 1;
            resultFrame.staging_framebuffer = stagingFramebuffer.data();
            resultFrame.staging_framebuffer_size = stagingFramebuffer.size();

            EXPECT_EQ(0, convert_frame(src.data(), src_format, src.size(),
                                       widthOrHeight, widthOrHeight,
                                       &resultFrame, 1.0f, 1.0f, 1.0f, 1.0f))
                    << "convert_frame, source=" << fourccToString(src_format)
                    << " dest=" << fourccToString(dest_format);
        }
    }
}

INSTANTIATE_TEST_CASE_P(CameraFormatConverters,
                        ConvertWithSize,
                        testing::Values(4, 8, 16, 18, 20, 32));

INSTANTIATE_TEST_CASE_P(CameraFormatConverters,
                        OddDimensions,
                        testing::Values(5, 7, 15, 17));
