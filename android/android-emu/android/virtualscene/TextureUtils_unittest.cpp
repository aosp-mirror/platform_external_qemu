// Copyright (C) 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/virtualscene/TextureUtils.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/ScopedStdioFile.h"
#include "android/utils/file_io.h"
#include "android/base/system/System.h"

#include <gtest/gtest.h>

#include <vector>

using namespace android::virtualscene;
using namespace android::base;

// For a 32x32 image, allow a max of half of the pixels to be off by one.
static constexpr double kCompareThreshold = 512.0;
static constexpr size_t kMaxVectorOutput = 128;

struct ImageTestParam {
    std::string filename;
    std::string goldenFilename;

    ImageTestParam(const char* filename, const char* goldenFilename)
        : filename(filename), goldenFilename(goldenFilename) {}
};

static void PrintTo(const ImageTestParam& param, std::ostream* os) {
    *os << "ImageTestParam(file=" << param.filename
        << ", golden=" << param.goldenFilename << ")";
}

namespace android {
namespace virtualscene {
static void PrintTo(const TextureUtils::Result& param, std::ostream* os) {
    *os << "TextureUtils::Result(" << param.mWidth << "x" << param.mHeight
        << ", format=";
    if (param.mFormat == TextureUtils::Format::RGB24) {
        *os << "RGB24";
    } else if (param.mFormat == TextureUtils::Format::RGBA32) {
        *os << "RGBA32";
    } else {
        *os << "<unknown format: " << static_cast<uint32_t>(param.mFormat)
            << ">";
    }
    *os << ", size=" << param.mBuffer.size() << " bytes)";
}
}  // namespace virtualscene
}  // namespace android

namespace std {
static void PrintTo(const vector<uint8_t>& vec, ostream* os) {
    ios::fmtflags origFlags(os->flags());

    *os << '{';
    size_t count = 0;
    for (vector<uint8_t>::const_iterator it = vec.begin(); it != vec.end();
         ++it, ++count) {
        if (count > 0) {
            *os << ", ";
        }

        if (count == kMaxVectorOutput) {
            *os << "... ";
            break;
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
}  // namespace std

static std::string testdataPathToAbsolute(StringView filename) {
    return PathUtils::join(System::get()->getProgramDirectory(), "testdata",
                           filename);
}

// Load an image using TextureUtils.
static void loadImage(StringView filename, TextureUtils::Result* outResult) {
    const std::string path = testdataPathToAbsolute(filename);
    Optional<TextureUtils::Result> result = TextureUtils::load(path.c_str());
    ASSERT_TRUE(result);
    *outResult = std::move(result.value());
}

static void loadGoldenBmp(StringView filename, TextureUtils::Result* result) {
    const std::string path = testdataPathToAbsolute(filename);

    constexpr size_t kBmpHeaderSize = 54;

    ScopedStdioFile fp(android_fopen(path.c_str(), "rb"));
    ASSERT_TRUE(fp) << "Failed to open golden image: " << path;

    uint8_t header[kBmpHeaderSize];
    ASSERT_EQ(kBmpHeaderSize, fread(header, 1, sizeof(header), fp.get()));
    ASSERT_EQ('B', header[0]);
    ASSERT_EQ('M', header[1]);

    uint32_t dataPos = *reinterpret_cast<uint32_t*>(&header[0x0A]);
    uint32_t imageSize = *reinterpret_cast<uint32_t*>(&header[0x22]);
    const uint16_t bitsPerPixel = *reinterpret_cast<uint16_t*>(&header[0x1C]);
    int width = *reinterpret_cast<int*>(&header[0x12]);
    int height = *reinterpret_cast<int*>(&header[0x16]);

    SCOPED_TRACE(::testing::Message()
                 << "dataPos=" << dataPos << ", imageSize=" << imageSize
                 << ", bitsPerPixel=" << bitsPerPixel << ", width=" << width
                 << ", height=" << height);

    if (height < 0) {
        height = -height;
    }

    if (imageSize == 0) {
        imageSize = width * height * 3;
    }

    if (dataPos < kBmpHeaderSize) {
        dataPos = kBmpHeaderSize;
    }

    ASSERT_TRUE(bitsPerPixel == 24 || bitsPerPixel == 32)
            << "Invalid bits per pixel: " << bitsPerPixel;

    result->mWidth = width;
    result->mHeight = height;
    result->mFormat = bitsPerPixel == 24 ? TextureUtils::Format::RGB24
                                         : TextureUtils::Format::RGBA32;

    std::vector<uint8_t>& data = result->mBuffer;

    data.resize(imageSize);
    ASSERT_EQ(0, fseek(fp.get(), dataPos, SEEK_SET));
    ASSERT_EQ(imageSize, fread(data.data(), 1, imageSize, fp.get()));
    fp.reset();

    const size_t bytesPerRow = bitsPerPixel / 8 * width;
    const size_t stride = (bytesPerRow + 3) / 4 * 4;

    if (bitsPerPixel == 32) {
        // Swizzle the data from ABGR to RGBA.
        for (size_t row = 0; row < height; ++row) {
            uint8_t* rowData = data.data() + row * stride;

            for (size_t i = 3; i < stride; i += 4) {
                const uint8_t a = rowData[i - 3];
                const uint8_t b = rowData[i - 2];
                rowData[i - 3] = rowData[i];
                rowData[i - 2] = rowData[i - 1];
                rowData[i - 1] = b;
                rowData[i] = a;
            }
        }
    } else {
        // Swizzle the data from BGR to RGB.
        for (size_t row = 0; row < height; ++row) {
            uint8_t* rowData = data.data() + row * stride;

            for (size_t i = 2; i < stride; i += 3) {
                const uint8_t tmp = rowData[i - 2];
                rowData[i - 2] = rowData[i];
                rowData[i] = tmp;
            }
        }
    }
}

static void compareSumOfSquaredDifferences(const std::vector<uint8_t>& image,
                                           const std::vector<uint8_t>& golden,
                                           double threshold) {
    ASSERT_EQ(golden.size(), image.size());

    double sum = 0.0;
    for (size_t i = 0; i < image.size(); ++i) {
        double diff = static_cast<double>(image[i]) - golden[i];
        sum += diff * diff;
    }

    EXPECT_LE(sum, threshold * image.size());
    if (sum > threshold * image.size()) {
        // Fall back to standard EXPECT_EQ for better error output.
        ASSERT_EQ(golden, image);
    }
}

class TextureUtilLoad : public ::testing::TestWithParam<ImageTestParam> {};

// Compare loaded images with golden versions.
TEST_P(TextureUtilLoad, Compare) {
    const ImageTestParam param = GetParam();

    TextureUtils::Result image;
    TextureUtils::Result golden;
    loadImage(param.filename, &image);
    loadGoldenBmp(param.goldenFilename, &golden);

    SCOPED_TRACE(::testing::Message()
                 << "image=" << ::testing::PrintToString(image) << "\n"
                 << " golden=" << ::testing::PrintToString(golden));

    ASSERT_EQ(golden.mWidth, image.mWidth);
    ASSERT_EQ(golden.mHeight, image.mHeight);
    ASSERT_EQ(golden.mFormat, image.mFormat);

    compareSumOfSquaredDifferences(image.mBuffer, golden.mBuffer,
                                   kCompareThreshold);
}

TEST(TextureUtilsBasic, InvalidExtension) {
    const std::string path = testdataPathToAbsolute("invalid.extension");
    ASSERT_FALSE(TextureUtils::load(path.c_str()));
}

TEST(TextureUtilsBasic, NoExtension) {
    const std::string path = testdataPathToAbsolute("invalid");
    ASSERT_FALSE(TextureUtils::load(path.c_str()));
}

TEST(TextureUtilsBasic, InvalidJPEG) {
    const std::string path = testdataPathToAbsolute("gray_alpha_golden.bmp");
    ASSERT_FALSE(TextureUtils::loadJPEG(path.c_str()));
}

TEST(TextureUtilsBasic, InvalidPNG) {
    const std::string path = testdataPathToAbsolute("gray_alpha_golden.bmp");
    ASSERT_FALSE(TextureUtils::loadPNG(path.c_str()));
}

INSTANTIATE_TEST_CASE_P(
        TextureUtils,
        TextureUtilLoad,
        ::testing::Values(
                ImageTestParam("gray_alpha.png", "gray_alpha_golden.bmp"),
                ImageTestParam("gray.png", "gray_golden.bmp"),
                ImageTestParam("indexed_alpha.png", "indexed_alpha_golden.bmp"),
                ImageTestParam("indexed.png", "indexed_golden.bmp"),
                ImageTestParam("interlaced.png", "interlaced_golden.bmp"),
                ImageTestParam("jpeg_gray.jpg", "jpeg_gray_golden.bmp"),
                ImageTestParam("jpeg_gray_progressive.jpg",
                               "jpeg_gray_progressive_golden.bmp"),
                ImageTestParam("jpeg_rgb24.jpg", "jpeg_rgb24_golden.bmp"),
                ImageTestParam("jpeg_rgb24_progressive.jpg",
                               "jpeg_rgb24_progressive_golden.bmp"),
                ImageTestParam("rgb24_31px.png", "rgb24_31px_golden.bmp"),
                ImageTestParam("rgba32.png", "rgba32_golden.bmp")));
