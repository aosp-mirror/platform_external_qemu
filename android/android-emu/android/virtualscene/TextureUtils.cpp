/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android/virtualscene/TextureUtils.h"
#include <string>
#include "android/base/files/PathUtils.h"
#include "android/base/files/ScopedStdioFile.h"
#include "android/utils/debug.h"

#include <png.h>
extern "C" {
#include <jpeglib.h>
}

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

using android::base::PathUtils;
using android::base::ScopedStdioFile;

namespace android {
namespace virtualscene {

bool TextureUtils::load(const char* filename,
                        std::vector<uint8_t>& buffer,
                        uint32_t* outWidth,
                        uint32_t* outHeight,
                        Format* outFormat) {
    *outWidth = 0;
    *outHeight = 0;
    *outFormat = Format::RGB24;

    const base::StringView extension = PathUtils::extension(filename);

    if (extension == ".png") {
        return loadPNG(filename, buffer, outWidth, outHeight, outFormat);
    } else if (extension == ".jpg" || extension == ".jpeg") {
        return loadJPEG(filename, buffer, outWidth, outHeight);
    } else {
        E("%s: Unsupported file format %s", __FUNCTION__,
          std::string(extension).c_str());
        return false;
    }
}

bool TextureUtils::loadPNG(const char* filename,
                           std::vector<uint8_t>& buffer,
                           uint32_t* outWidth,
                           uint32_t* outHeight,
                           Format* outFormat) {
    *outWidth = 0;
    *outHeight = 0;
    *outFormat = Format::RGB24;

    ScopedStdioFile fp(fopen(filename, "rb"));
    if (!fp) {
        E("%s: Failed to open file %s", __FUNCTION__, filename);
        return false;
    }

    uint8_t header[8];
    if (fread(header, sizeof(header), 1, fp.get()) != 1) {
        E("%s: Failed to read header", __FUNCTION__);
        return false;
    }

    if (png_sig_cmp(header, 0, sizeof(header))) {
        E("%s: header is not a PNG header", __FUNCTION__);
        return false;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!png) {
        E("%s: Failed to allocate png read struct", __FUNCTION__);
        return false;
    }

    png_infop pngInfo = png_create_info_struct(png);
    if (!pngInfo) {
        E("%s: Failed to allocate png info struct", __FUNCTION__);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        E("%s: PNG library error", __FUNCTION__);
        png_destroy_read_struct(&png, &pngInfo, 0);
        return false;
    }

    png_init_io(png, fp.get());
    png_set_sig_bytes(png, 8);

    png_read_info(png, pngInfo);

    png_uint_32 width = 0;
    png_uint_32 height = 0;
    int bitDepth = 0;
    int colorType = 0;
    png_get_IHDR(png, pngInfo, &width, &height, &bitDepth, &colorType, nullptr,
                 nullptr, nullptr);
    D("%s: Loaded PNG %s, %dx%d (d=%d, c=%d)", __FUNCTION__, filename, width,
      height, bitDepth, colorType);

    // Convert to RGB if required.
    if (colorType == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png);
    } else if (colorType == PNG_COLOR_TYPE_GRAY ||
               colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png);
    }

    if (png_get_valid(png, pngInfo, PNG_INFO_tRNS)) {
        // Convert palette and grayscale transparency to full alpha channel.
        png_set_tRNS_to_alpha(png);
    }

    // At this point, the bit depth is either 8 or 16, ensure 8 bpp.
    if (bitDepth == 16) {
        png_set_strip_16(png);
    }

    // Update the pngInfo struct and validate that we have a format that we can
    // handle.
    png_read_update_info(png, pngInfo);

    const int newBitDepth = png_get_bit_depth(png, pngInfo);
    const int newColorType = png_get_color_type(png, pngInfo);
    if (newBitDepth != bitDepth || newColorType != colorType) {
        D("%s: Converting PNG to (d=%d, c=%d)", __FUNCTION__, newBitDepth,
          newColorType);
    }

    if (newColorType != PNG_COLOR_TYPE_RGB &&
        newColorType != PNG_COLOR_TYPE_RGB_ALPHA) {
        E("%s: Unsupported color type: %d", __FUNCTION__, newColorType);
        png_destroy_read_struct(&png, &pngInfo, 0);
        return false;
    }

    const size_t rowBytes = png_get_rowbytes(png, pngInfo);
    const size_t stride = (rowBytes + 3) / 4 * 4;  // Align to 4-byte boundary.
    std::vector<uint8_t> data(stride * height);
    std::vector<uint8_t*> rowPtrs(height);

    for (size_t i = 0; i < height; i++) {
        rowPtrs[i] = data.data() + stride * (height - i - 1);
    }

    png_read_image(png, rowPtrs.data());
    png_destroy_read_struct(&png, &pngInfo, 0);

    buffer = std::move(data);

    *outWidth = width;
    *outHeight = height;
    *outFormat =
            newColorType == PNG_COLOR_TYPE_RGB ? Format::RGB24 : Format::RGBA32;

    return true;
}

struct ErrorManager {
    struct jpeg_error_mgr pub;  // Public fields.
    jmp_buf setjmp_buffer;
};

bool TextureUtils::loadJPEG(const char* filename,
                            std::vector<uint8_t>& buffer,
                            uint32_t* outWidth,
                            uint32_t* outHeight) {
    *outWidth = 0;
    *outHeight = 0;

    ScopedStdioFile fp(fopen(filename, "rb"));
    if (!fp) {
        E("%s: Failed to open file %s", __FUNCTION__, filename);
        return false;
    }

    jpeg_decompress_struct cinfo;

    // Set up normal error routines, then override error_exit to avoid exit()
    // on failure.
    ErrorManager jerr;
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = [](j_common_ptr cinfoPtr) {
        ErrorManager* err = reinterpret_cast<ErrorManager*>(cinfoPtr->err);
        longjmp(err->setjmp_buffer, 1);
    };

    if (setjmp(jerr.setjmp_buffer)) {
        E("%s: JPEG library error", __FUNCTION__);
        // Prints the message.
        (cinfo.err->output_message)(
                reinterpret_cast<jpeg_common_struct*>(&cinfo));
        jpeg_destroy_decompress(&cinfo);
        return false;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp.get());

    // We can safely ignore the return value since we're using a stdio source
    // and passing require_image as true.
    (void)jpeg_read_header(&cinfo, /* require_image */ TRUE);
    cinfo.out_color_space = JCS_RGB;  // Force RGB format.
    (void)jpeg_start_decompress(&cinfo);

    const uint32_t width = cinfo.output_width;
    const uint32_t height = cinfo.output_height;
    D("%s: Loaded JPEG %s, %dx%d", __FUNCTION__, filename, width, height);

    if (cinfo.output_components != 3) {
        E("%s: Unsupported output_components %d, should be 3.", __FUNCTION__,
          cinfo.output_components);
        jpeg_destroy_decompress(&cinfo);
        return false;
    }

    const size_t rowBytes = width * cinfo.output_components;
    const size_t stride = (rowBytes + 3) / 4 * 4;  // Align to 4-byte boundary.
    std::vector<uint8_t> data(stride * height);

    while (cinfo.output_scanline < height) {
        // libjpeg can read multiple scanlines at a time, but on high-quality
        // decompression it is typically one at a time.  Only read one at a time
        // for simplicity.
        uint8_t* rowPtrs[1];
        rowPtrs[0] =
                data.data() + (height - cinfo.output_scanline - 1) * stride;
        (void)jpeg_read_scanlines(&cinfo, rowPtrs, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    buffer = std::move(data);
    *outWidth = width;
    *outHeight = height;

    return true;
}

}  // namespace virtualscene
}  // namespace android
