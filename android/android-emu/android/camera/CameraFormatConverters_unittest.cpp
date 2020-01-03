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

#include <gtest/gtest.h>

#include <vector>

// An arbitrary color that's easily recognizable in hex and different for each
// channel.
static constexpr uint8_t kAlpha = 0xFF;
static constexpr uint8_t kRed = 0x00;
static constexpr uint8_t kGreen = 0x66;
static constexpr uint8_t kBlue = 0xCC;

static constexpr float kDefaultColorScale = 1.0f;
static constexpr float kDefaultExpComp = 1.0f;

// Difference squared per pixel.  Libyuv produces slightly different color
// values for each pixel, allow a slight difference.
static constexpr double kDifferenceSq = 2.0;

// For exposure compensation, allow a slightly more lenient error to compensate
// for rounding errors.
static constexpr double kLenientDifferenceSq = 4.0;

// Max value that a pixel could differ, used to exclude edge pixel differences.
static constexpr double kCompletelyDifferentSq = 256.0 * 256.0;

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

enum class FramebufferCompare { Default, IgnoreEdges };

struct FramebufferSizeParam {
    int width;
    int height;
    FramebufferCompare options;

    FramebufferSizeParam(
            int width,
            int height,
            FramebufferCompare options = FramebufferCompare::Default)
        : width(width), height(height), options(options) {}

    double getThreshold() const {
        if (options == FramebufferCompare::IgnoreEdges) {
            return lerpThreshold(kDifferenceSq, kCompletelyDifferentSq,
                                 edgePixelPercent());
        } else {
            return kDifferenceSq;
        }
    }

    double getThresholdForResize() const {
        // Ignore edges and increase the maximum difference since this is for
        // two convert operations.
        return lerpThreshold(kDifferenceSq * 2, kCompletelyDifferentSq,
                             edgePixelPercent());
    }

private:
    // Edge pixels are the bottom- and right-most rows/columns. For YUV images,
    // there are three planes (each at half-resolution), so for the last row
    // (width) + 2 * (width / 2) pixels can be different. This is simplified to
    // 2 * width.
    double edgePixelPercent() const {
        return (2.0 * width + 2.0 * height) / (width * height);
    }

    static double lerpThreshold(double a, double b, double t) {
        return (1.0 - t) * a + t * b;
    }
};

static void PrintTo(const FramebufferSizeParam& param, std::ostream* os) {
    *os << "FramebufferSizeParam(" << param.width << "x" << param.height
        << ", threshold=" << param.getThreshold() << ")";
}

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
                                    width, height, &framebuffer, 1,
                                    kDefaultColorScale, kDefaultColorScale,
                                    kDefaultColorScale, kDefaultExpComp))
            << "convert_frame_slow, source="
            << fourccToString(V4L2_PIX_FMT_ARGB32)
            << " dest=" << fourccToString(format);

    return dest;
}

static void compareSumOfSquaredDifferences(const std::vector<uint8_t>& src,
                                           const std::vector<uint8_t>& dest,
                                           double threshold) {
    ASSERT_EQ(src.size(), dest.size());

    double sum = 0.0;
    for (size_t i = 0; i < src.size(); ++i) {
        double diff = static_cast<double>(src[i]) - dest[i];
        sum += diff * diff;
    }

    EXPECT_LE(sum, threshold * src.size());
    if (sum > threshold * src.size()) {
        // Fall back to standard EXPECT_EQ for better error output.
        ASSERT_EQ(src, dest);
    }
}

class ConvertWithSize : public testing::TestWithParam<FramebufferSizeParam> {};

// Validate identity conversions.
TEST_P(ConvertWithSize, SlowPathIdentity) {
    const FramebufferSizeParam param = GetParam();

    for (uint32_t format : kSupportedDestinationFormats) {
        SCOPED_TRACE(testing::Message() << "source=" << fourccToString(format)
                                        << " dest=" << fourccToString(format));

        std::vector<uint8_t> src = generateFramebuffer(
                format, param.width, param.height, kAlpha, kRed, kGreen, kBlue);
        const size_t destSize = bufferSize(format, param.width, param.height);
        std::vector<uint8_t> dest(destSize);

        ClientFrameBuffer framebuffer = {};
        framebuffer.pixel_format = format;
        framebuffer.framebuffer = dest.data();

        EXPECT_EQ(0,
                  convert_frame_slow(src.data(), format, src.size(),
                                     param.width, param.height, &framebuffer, 1,
                                     kDefaultColorScale, kDefaultColorScale,
                                     kDefaultColorScale, kDefaultExpComp));

        compareSumOfSquaredDifferences(src, dest, param.getThreshold());

        if (HasFatalFailure()) {
            return;
        }
    }
}

TEST_P(ConvertWithSize, FastPathIdentity) {
    const FramebufferSizeParam param = GetParam();
    uint8_t* stagingFramebuffer = nullptr;
    size_t stagingFramebufferSize = 0;

    for (uint32_t format : kSupportedDestinationFormats) {
        SCOPED_TRACE(testing::Message() << "source=" << fourccToString(format)
                                        << " dest=" << fourccToString(format));

        std::vector<uint8_t> src = generateFramebuffer(
                format, param.width, param.height, kAlpha, kRed, kGreen, kBlue);
        const size_t destSize = bufferSize(format, param.width, param.height);
        std::vector<uint8_t> dest(destSize);

        ClientFrameBuffer framebuffer = {};
        framebuffer.pixel_format = format;
        framebuffer.framebuffer = dest.data();

        ClientFrame resultFrame = {};
        resultFrame.framebuffers = &framebuffer;
        resultFrame.framebuffers_count = 1;
        resultFrame.staging_framebuffer = &stagingFramebuffer;
        resultFrame.staging_framebuffer_size = &stagingFramebufferSize;

        EXPECT_EQ(0, convert_frame_fast(src.data(), format, src.size(),
                                        param.width, param.height, param.width,
                                        param.height, &resultFrame,
                                        kDefaultExpComp));

        compareSumOfSquaredDifferences(src, dest, param.getThreshold());

        if (HasFatalFailure()) {
            return;
        }
    }

    free(stagingFramebuffer);
}

TEST_P(ConvertWithSize, Resize) {
    static constexpr size_t kScaleAmount = 4;

    const FramebufferSizeParam param = GetParam();
    uint8_t* stagingFramebuffer = nullptr;
    size_t stagingFramebufferSize = 0;

    for (uint32_t format : kSupportedDestinationFormats) {
        SCOPED_TRACE(testing::Message() << "source=" << fourccToString(format)
                                        << " dest=" << fourccToString(format));

        std::vector<uint8_t> src = generateFramebuffer(
                format, param.width, param.height, kAlpha, kRed, kGreen, kBlue);

        const size_t intermediateSize =
                bufferSize(format, param.width / kScaleAmount,
                           param.height / kScaleAmount);
        std::vector<uint8_t> intermediate(intermediateSize);

        const size_t destSize = bufferSize(format, param.width, param.height);
        std::vector<uint8_t> dest(destSize);

        ClientFrameBuffer framebuffer = {};
        framebuffer.pixel_format = format;
        framebuffer.framebuffer = intermediate.data();

        ClientFrame resultFrame = {};
        resultFrame.framebuffers = &framebuffer;
        resultFrame.framebuffers_count = 1;
        resultFrame.staging_framebuffer = &stagingFramebuffer;
        resultFrame.staging_framebuffer_size = &stagingFramebufferSize;

        EXPECT_EQ(0, convert_frame_fast(src.data(), format, src.size(),
                                        param.width, param.height,
                                        param.width / kScaleAmount,
                                        param.height / kScaleAmount,
                                        &resultFrame, kDefaultExpComp));

        memset(stagingFramebuffer, 0, stagingFramebufferSize);

        framebuffer.framebuffer = dest.data();
        EXPECT_EQ(0, convert_frame_fast(
                             intermediate.data(), format, intermediate.size(),
                             param.width / kScaleAmount,
                             param.height / kScaleAmount, param.width,
                             param.height, &resultFrame, kDefaultExpComp));

        compareSumOfSquaredDifferences(src, dest,
                                       param.getThresholdForResize());

        if (HasFatalFailure()) {
            return;
        }
    }

    free(stagingFramebuffer);
}

// Validate conversions from all source formats to all dest formats.
TEST_P(ConvertWithSize, SourceToDest) {
    const FramebufferSizeParam param = GetParam();
    uint8_t* stagingFramebuffer = nullptr;
    size_t stagingFramebufferSize = 0;

    for (uint32_t src_format : kSupportedSourceFormats) {
        std::vector<uint8_t> src =
                generateFramebuffer(src_format, param.width, param.height,
                                    kAlpha, kRed, kGreen, kBlue);

        for (uint32_t dest_format : kSupportedDestinationFormats) {
            SCOPED_TRACE(testing::Message()
                         << "source=" << fourccToString(src_format)
                         << " dest=" << fourccToString(dest_format));

            const size_t destSize =
                    bufferSize(dest_format, param.width, param.height);
            std::vector<uint8_t> dest(destSize);

            ClientFrameBuffer framebuffer = {};
            framebuffer.pixel_format = dest_format;
            framebuffer.framebuffer = dest.data();

            ClientFrame resultFrame = {};
            resultFrame.framebuffers = &framebuffer;
            resultFrame.framebuffers_count = 1;
            resultFrame.staging_framebuffer = &stagingFramebuffer;
            resultFrame.staging_framebuffer_size = &stagingFramebufferSize;

            EXPECT_EQ(0, convert_frame(src.data(), src_format, src.size(),
                                       param.width, param.height, &resultFrame,
                                       kDefaultColorScale, kDefaultColorScale,
                                       kDefaultColorScale, kDefaultExpComp));

            std::vector<uint8_t> destBaseline(destSize);
            ClientFrameBuffer baselineFramebuffer = {};
            baselineFramebuffer.pixel_format = dest_format;
            baselineFramebuffer.framebuffer = destBaseline.data();

            EXPECT_EQ(0,
                      convert_frame_slow(src.data(), src_format, src.size(),
                                         param.width, param.height,
                                         &baselineFramebuffer, 1,
                                         kDefaultColorScale, kDefaultColorScale,
                                         kDefaultColorScale, kDefaultExpComp));

            compareSumOfSquaredDifferences(destBaseline, dest,
                                           param.getThreshold());

            if (HasFatalFailure()) {
                return;
            }
        }
    }

    free(stagingFramebuffer);
}

INSTANTIATE_TEST_CASE_P(
        CameraFormatConverters,
        ConvertWithSize,
        testing::Values(
                // Even dimensions may have slight differences per
                // pixel.
                FramebufferSizeParam(4, 4),
                FramebufferSizeParam(8, 8),
                FramebufferSizeParam(16, 16),
                FramebufferSizeParam(18, 18),
                FramebufferSizeParam(20, 20),
                FramebufferSizeParam(32, 32),
                // The fast and slow path handle boundary pixels for
                // odd dimensions differently, allow them to be
                // different with a larger threshold.
                FramebufferSizeParam(5, 5, FramebufferCompare::IgnoreEdges),
                FramebufferSizeParam(7, 7, FramebufferCompare::IgnoreEdges),
                FramebufferSizeParam(15, 15, FramebufferCompare::IgnoreEdges),
                FramebufferSizeParam(17, 17, FramebufferCompare::IgnoreEdges)));

class FrameModifiers : public testing::TestWithParam<float> {};

TEST_P(FrameModifiers, ExpComp) {
    constexpr int kWidth = 4;
    constexpr int kHeight = 4;

    const float expComp = GetParam();

    uint8_t* stagingFramebuffer = nullptr;
    size_t stagingFramebufferSize = 0;

    for (uint32_t src_format : kSupportedSourceFormats) {
        std::vector<uint8_t> src = generateFramebuffer(
                src_format, kWidth, kHeight, kAlpha, kRed, kGreen, kBlue);

        for (uint32_t dest_format : kSupportedDestinationFormats) {
            SCOPED_TRACE(testing::Message()
                         << "source=" << fourccToString(src_format)
                         << " dest=" << fourccToString(dest_format));

            const size_t destSize = bufferSize(dest_format, kWidth, kHeight);
            std::vector<uint8_t> dest(destSize);

            ClientFrameBuffer framebuffer = {};
            framebuffer.pixel_format = dest_format;
            framebuffer.framebuffer = dest.data();

            ClientFrame resultFrame = {};
            resultFrame.framebuffers = &framebuffer;
            resultFrame.framebuffers_count = 1;
            resultFrame.staging_framebuffer = &stagingFramebuffer;
            resultFrame.staging_framebuffer_size = &stagingFramebufferSize;

            EXPECT_EQ(0, convert_frame(src.data(), src_format, src.size(),
                                       kWidth, kHeight, &resultFrame,
                                       kDefaultColorScale, kDefaultColorScale,
                                       kDefaultColorScale, expComp));

            std::vector<uint8_t> destBaseline(destSize);
            ClientFrameBuffer baselineFramebuffer = {};
            baselineFramebuffer.pixel_format = dest_format;
            baselineFramebuffer.framebuffer = destBaseline.data();

            EXPECT_EQ(0, convert_frame_slow(
                                 src.data(), src_format, src.size(), kWidth,
                                 kHeight, &baselineFramebuffer, 1,
                                 kDefaultColorScale, kDefaultColorScale,
                                 kDefaultColorScale, expComp));

            compareSumOfSquaredDifferences(destBaseline, dest,
                                           kLenientDifferenceSq);

            if (HasFatalFailure()) {
                return;
            }
        }
    }

    free(stagingFramebuffer);
}

INSTANTIATE_TEST_CASE_P(CameraFormatConverters,
                        FrameModifiers,
                        testing::Values(1.0f, 0.0f, 0.5f, -1.0f, 2.0f));
