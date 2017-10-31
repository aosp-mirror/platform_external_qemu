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

#include "android/virtualscene/VirtualSceneTexture.h"
#include "android/base/files/PathUtils.h"
#include "android/base/files/ScopedStdioFile.h"
#include "android/base/system/System.h"
#include "android/utils/debug.h"
#include <string>

#include <png.h>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

using android::base::PathUtils;
using android::base::ScopedStdioFile;
using android::base::System;

namespace android {
namespace virtualscene {

VirtualSceneTexture::VirtualSceneTexture(const GLESv2Dispatch* gles2,
                                         GLuint textureId)
    : mGles2(gles2), mTextureId(textureId) {
}

VirtualSceneTexture::~VirtualSceneTexture() {
    if (mGles2) {
        mGles2->glDeleteTextures(1, &mTextureId);
    }
}

std::unique_ptr<VirtualSceneTexture> VirtualSceneTexture::load(
        const GLESv2Dispatch* gles2,
        const char* filename) {
    GLint maxTextureSize = 0;
    gles2->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

    std::vector<uint8_t> buffer;
    uint32_t width = 0;
    uint32_t height = 0;
    if (!loadPNG(filename, buffer, &width, &height)) {
        return nullptr;
    }

    if (width == 0 || height == 0 || width > maxTextureSize ||
        height > maxTextureSize) {
        E("%s: Invalid texture size, %d x %d, GL_MAX_TEXTURE_SIZE = %d", width,
          height, maxTextureSize);
        return nullptr;
    }

    GLuint textureId;
    gles2->glGenTextures(1, &textureId);
    gles2->glBindTexture(GL_TEXTURE_2D, textureId);
    gles2->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                        GL_UNSIGNED_BYTE, buffer.data());
    gles2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    gles2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    gles2->glBindTexture(GL_TEXTURE_2D, 0);

#ifdef DEBUG
    // Only check for GL error on debug builds to avoid a potential synchronous
    // flush.
    const GLenum error = gles2->glGetError();
    if (error != GL_NO_ERROR) {
        E("%s: GL error %d", __FUNCTION__, error);
        return nullptr;
    }
#endif

    return std::unique_ptr<VirtualSceneTexture>(
            new VirtualSceneTexture(gles2, textureId));
}

GLuint VirtualSceneTexture::getTextureId() const {
    return mTextureId;
}

bool VirtualSceneTexture::loadPNG(const char* filename,
                                  std::vector<uint8_t>& buffer,
                                  uint32_t* outWidth,
                                  uint32_t* outHeight) {
    *outWidth = 0;
    *outHeight = 0;

    const std::string path = PathUtils::join(
            System::get()->getLauncherDirectory(), "resources", filename);

    uint8_t header[8];
    std::vector<uint8_t> data;
    std::vector<uint8_t*> rowPtrs;

    png_uint_32 width = 0;
    png_uint_32 height = 0;

    int bitdepth, colortype, imethod, cmethod, fmethod;

    ScopedStdioFile fp(fopen(path.c_str(), "rb"));
    if (!fp) {
        E("%s: Failed to open file %s", __FUNCTION__, path.c_str());
        return false;
    }

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

    png_get_IHDR(png, pngInfo, &width, &height, &bitdepth, &colortype, &imethod,
                 &cmethod, &fmethod);
    D("%s: Loaded PNG %s, %d x %d (d=%d, c=%d)", __FUNCTION__, filename, width,
      height, bitdepth, colortype);

    switch (colortype) {
        case PNG_COLOR_TYPE_PALETTE:
            png_set_palette_to_rgb(png);
            break;

        case PNG_COLOR_TYPE_RGB:
            if (png_get_valid(png, pngInfo, PNG_INFO_tRNS)) {
                png_set_tRNS_to_alpha(png);
            } else {
                png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
            }
            break;

        case PNG_COLOR_TYPE_RGB_ALPHA:
            break;

        case PNG_COLOR_TYPE_GRAY:
            if (bitdepth < 8) {
                png_set_expand_gray_1_2_4_to_8(png);
            }
            break;

        default:
            E("%s: unsupported color type: %d", __FUNCTION__, colortype);
            png_destroy_read_struct(&png, &pngInfo, 0);
            return false;
    }

    if (bitdepth == 16) {
        png_set_strip_16(png);
    }

    data.resize(width * height * 4);
    rowPtrs.resize(height);

    for (size_t i = 0; i < height; i++) {
        rowPtrs[i] = data.data() + ((width * 4) * i);
    }

    png_read_image(png, rowPtrs.data());
    png_destroy_read_struct(&png, &pngInfo, 0);

    buffer = std::move(data);

    *outWidth = width;
    *outHeight = height;

    return true;
}

}  // namespace virtualscene
}  // namespace android
