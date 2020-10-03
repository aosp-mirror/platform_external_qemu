/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "GLcommon/SaveableTexture.h"

#include "android/base/ArraySize.h"
#include "android/base/containers/SmallVector.h"
#include "android/base/files/StreamSerializing.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/Profiler.h"
#include "android/base/StringView.h"
#include "android/base/system/System.h"

#include "GLcommon/GLEScontext.h"
#include "GLcommon/GLutils.h"
#include "GLcommon/TextureUtils.h"

#include "emugl/common/crash_reporter.h"
#include "emugl/common/logging.h"

#include <algorithm>

#define SAVEABLE_TEXTURE_DEBUG 0

#if SAVEABLE_TEXTURE_DEBUG
#define D(fmt,...) printf("%s:%d " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);
#else
#define D(fmt,...)
#endif

using android::base::ScopedMemoryProfiler;
using android::base::LazyInstance;
using android::base::MemoryProfiler;
using android::base::StringView;

static const GLenum kTexParam[] = {
    GL_TEXTURE_MIN_FILTER,
    GL_TEXTURE_MAG_FILTER,
    GL_TEXTURE_WRAP_S,
    GL_TEXTURE_WRAP_T,
};

static const GLenum kTexParamGles3[] = {
    GL_TEXTURE_BASE_LEVEL,
    GL_TEXTURE_COMPARE_FUNC,
    GL_TEXTURE_COMPARE_MODE,
    GL_TEXTURE_MIN_LOD,
    GL_TEXTURE_MAX_LOD,
    GL_TEXTURE_MAX_LEVEL,
    GL_TEXTURE_SWIZZLE_R,
    GL_TEXTURE_SWIZZLE_G,
    GL_TEXTURE_SWIZZLE_B,
    GL_TEXTURE_SWIZZLE_A,
    GL_TEXTURE_WRAP_R,
};

static uint32_t s_texAlign(uint32_t v, uint32_t align) {
    uint32_t rem = v % align;
    return rem ? (v + (align - rem)) : v;
}

// s_computePixelSize is both in the host and the guest. Consider moving it to
// android-emugl/shared

static int s_computePixelSize(GLenum format, GLenum type) {
#define FORMAT_ERROR(format, type)                                         \
    fprintf(stderr, "%s:%d unknown format/type 0x%x 0x%x\n", __FUNCTION__, \
            __LINE__, format, type);

    switch (type) {
        case GL_BYTE:
            switch (format) {
                case GL_R8:
                case GL_R8I:
                case GL_R8_SNORM:
                case GL_RED:
                    return 1;
                case GL_RED_INTEGER:
                    return 1;
                case GL_RG8:
                case GL_RG8I:
                case GL_RG8_SNORM:
                case GL_RG:
                    return 1 * 2;
                case GL_RG_INTEGER:
                    return 1 * 2;
                case GL_RGB8:
                case GL_RGB8I:
                case GL_RGB8_SNORM:
                case GL_RGB:
                    return 1 * 3;
                case GL_RGB_INTEGER:
                    return 1 * 3;
                case GL_RGBA8:
                case GL_RGBA8I:
                case GL_RGBA8_SNORM:
                case GL_RGBA:
                    return 1 * 4;
                case GL_RGBA_INTEGER:
                    return 1 * 4;
                default:
                    FORMAT_ERROR(format, type);
            }
            break;
        case GL_UNSIGNED_BYTE:
            switch (format) {
                case GL_R8:
                case GL_R8UI:
                case GL_RED:
                    return 1;
                case GL_RED_INTEGER:
                    return 1;
                case GL_ALPHA8_EXT:
                case GL_ALPHA:
                    return 1;
                case GL_LUMINANCE8_EXT:
                case GL_LUMINANCE:
                    return 1;
                case GL_LUMINANCE8_ALPHA8_EXT:
                case GL_LUMINANCE_ALPHA:
                    return 1 * 2;
                case GL_RG8:
                case GL_RG8UI:
                case GL_RG:
                    return 1 * 2;
                case GL_RG_INTEGER:
                    return 1 * 2;
                case GL_RGB8:
                case GL_RGB8UI:
                case GL_SRGB8:
                case GL_RGB:
                    return 1 * 3;
                case GL_RGB_INTEGER:
                    return 1 * 3;
                case GL_RGBA8:
                case GL_RGBA8UI:
                case GL_SRGB8_ALPHA8:
                case GL_RGBA:
                    return 1 * 4;
                case GL_RGBA_INTEGER:
                    return 1 * 4;
                case GL_BGRA_EXT:
                case GL_BGRA8_EXT:
                    return 1 * 4;
                default:
                    FORMAT_ERROR(format, type);
            }
            break;
        case GL_SHORT:
            switch (format) {
                case GL_R16I:
                case GL_RED_INTEGER:
                    return 2;
                case GL_RG16I:
                case GL_RG_INTEGER:
                    return 2 * 2;
                case GL_RGB16I:
                case GL_RGB_INTEGER:
                    return 2 * 3;
                case GL_RGBA16I:
                case GL_RGBA_INTEGER:
                    return 2 * 4;
                default:
                    FORMAT_ERROR(format, type);
            }
            break;
        case GL_UNSIGNED_SHORT:
            switch (format) {
                case GL_DEPTH_COMPONENT16:
                case GL_DEPTH_COMPONENT:
                    return 2;
                case GL_R16UI:
                case GL_RED_INTEGER:
                    return 2;
                case GL_RG16UI:
                case GL_RG_INTEGER:
                    return 2 * 2;
                case GL_RGB16UI:
                case GL_RGB_INTEGER:
                    return 2 * 3;
                case GL_RGBA16UI:
                case GL_RGBA_INTEGER:
                    return 2 * 4;
                default:
                    FORMAT_ERROR(format, type);
            }
            break;
        case GL_INT:
            switch (format) {
                case GL_R32I:
                case GL_RED_INTEGER:
                    return 4;
                case GL_RG32I:
                case GL_RG_INTEGER:
                    return 4 * 2;
                case GL_RGB32I:
                case GL_RGB_INTEGER:
                    return 4 * 3;
                case GL_RGBA32I:
                case GL_RGBA_INTEGER:
                    return 4 * 4;
                default:
                    FORMAT_ERROR(format, type);
            }
            break;
        case GL_UNSIGNED_INT:
            switch (format) {
                case GL_DEPTH_COMPONENT16:
                case GL_DEPTH_COMPONENT24:
                case GL_DEPTH_COMPONENT32_OES:
                case GL_DEPTH_COMPONENT:
                    return 4;
                case GL_R32UI:
                case GL_RED_INTEGER:
                    return 4;
                case GL_RG32UI:
                case GL_RG_INTEGER:
                    return 4 * 2;
                case GL_RGB32UI:
                case GL_RGB_INTEGER:
                    return 4 * 3;
                case GL_RGBA32UI:
                case GL_RGBA_INTEGER:
                    return 4 * 4;
                default:
                    FORMAT_ERROR(format, type);
            }
            break;
        case GL_UNSIGNED_SHORT_4_4_4_4:
        case GL_UNSIGNED_SHORT_5_5_5_1:
        case GL_UNSIGNED_SHORT_5_6_5:
        case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT:
        case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT:
            return 2;
        case GL_UNSIGNED_INT_10F_11F_11F_REV:
        case GL_UNSIGNED_INT_5_9_9_9_REV:
        case GL_UNSIGNED_INT_2_10_10_10_REV:
        case GL_UNSIGNED_INT_24_8_OES:
            return 4;
        case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
            return 4 + 4;
        case GL_FLOAT:
            switch (format) {
                case GL_DEPTH_COMPONENT32F:
                case GL_DEPTH_COMPONENT:
                    return 4;
                case GL_ALPHA32F_EXT:
                case GL_ALPHA:
                    return 4;
                case GL_LUMINANCE32F_EXT:
                case GL_LUMINANCE:
                    return 4;
                case GL_LUMINANCE_ALPHA32F_EXT:
                case GL_LUMINANCE_ALPHA:
                    return 4 * 2;
                case GL_RED:
                    return 4;
                case GL_R32F:
                    return 4;
                case GL_RG:
                    return 4 * 2;
                case GL_RG32F:
                    return 4 * 2;
                case GL_RGB:
                    return 4 * 3;
                case GL_RGB32F:
                    return 4 * 3;
                case GL_RGBA:
                    return 4 * 4;
                case GL_RGBA32F:
                    return 4 * 4;
                default:
                    FORMAT_ERROR(format, type);
            }
            break;
        case GL_HALF_FLOAT:
        case GL_HALF_FLOAT_OES:
            switch (format) {
                case GL_ALPHA16F_EXT:
                case GL_ALPHA:
                    return 2;
                case GL_LUMINANCE16F_EXT:
                case GL_LUMINANCE:
                    return 2;
                case GL_LUMINANCE_ALPHA16F_EXT:
                case GL_LUMINANCE_ALPHA:
                    return 2 * 2;
                case GL_RED:
                    return 2;
                case GL_R16F:
                    return 2;
                case GL_RG:
                    return 2 * 2;
                case GL_RG16F:
                    return 2 * 2;
                case GL_RGB:
                    return 2 * 3;
                case GL_RGB16F:
                    return 2 * 3;
                case GL_RGBA:
                    return 2 * 4;
                case GL_RGBA16F:
                    return 2 * 4;
                default:
                    FORMAT_ERROR(format, type);
            }
            break;
        default:
            FORMAT_ERROR(format, type);
    }

    return 0;
}

static uint32_t s_texImageSize(GLenum internalformat,
                               GLenum type,
                               int unpackAlignment,
                               GLsizei width,
                               GLsizei height) {
    uint32_t alignedWidth = s_texAlign(width, unpackAlignment);
    uint32_t pixelSize = s_computePixelSize(internalformat, type);
    uint32_t totalSize = pixelSize * alignedWidth * height;

    return totalSize;
}

struct TextureDataReader {
    GLESVersion glesVersion = GLES_2_0;
    GLenum fbTarget = GL_FRAMEBUFFER;
    GLint prevViewport[4] = { 0, 0, 0, 0 };
    GLuint fbo = 0;
    GLuint prevFbo = 0;

    void setupFbo() {
        GLenum fbBindingTarget;
        auto gl = GLEScontext::dispatcher();

        glesVersion = gl.getGLESVersion();
        if (glesVersion >= GLES_3_0) {
            fbTarget = GL_READ_FRAMEBUFFER;
            fbBindingTarget = GL_READ_FRAMEBUFFER_BINDING;
        } else {
            fbTarget = GL_FRAMEBUFFER;
            fbBindingTarget = GL_FRAMEBUFFER_BINDING;
        }
        gl.glGetIntegerv(GL_VIEWPORT, prevViewport);
        gl.glGenFramebuffers(1, &fbo);
        gl.glGetIntegerv(fbBindingTarget, (GLint*)&prevFbo);
        gl.glBindFramebuffer(fbTarget, fbo);
    }

    void teardownFbo() {
        if (!fbo) return;
        auto gl = GLEScontext::dispatcher();

        gl.glBindFramebuffer(fbTarget, prevFbo);
        gl.glDeleteFramebuffers(1, &fbo);
        gl.glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);

        *this = {};
    }

    bool shouldUseReadPixels(
        GLenum target, GLenum level, GLenum format, GLenum type) {
        // TODO: if (isGles2Gles()) return true

        // TODO: Query extensions for support for these kinds of things
        if (target != GL_TEXTURE_2D || level != 0) return false;

#define KNOWN_GOOD_READ_PIXELS_COMBINATION(goodFormat, goodType) \
        if (format == goodFormat && type == goodType) return true;

        KNOWN_GOOD_READ_PIXELS_COMBINATION(GL_RGB, GL_UNSIGNED_BYTE)
        KNOWN_GOOD_READ_PIXELS_COMBINATION(GL_RGBA, GL_UNSIGNED_BYTE)

        return false;
    }

    void getTexImage(
        GLuint globalName, GLenum target, GLenum level, GLenum format, GLenum type,
        GLint width, GLint height, GLint depth, uint8_t* data) {

        D("Reading: %u 0x%x %u 0x%x 0x%x %d x %d x %d...",
          globalName, target, level, format, type, width, height, depth);

        auto gl = GLEScontext::dispatcher();

        if (!shouldUseReadPixels(target, level, format, type)) {
            D("with underlying glGetTexImage");
            gl.glGetTexImage(target, level, format, type, data);
            return;
        }


        GLenum attachment = GL_COLOR_ATTACHMENT0;

        switch (format) {
            case GL_DEPTH_COMPONENT:
                attachment = GL_DEPTH_ATTACHMENT;
                break;
            case GL_DEPTH_STENCIL:
                attachment = GL_DEPTH_STENCIL_ATTACHMENT;
                break;
        }

        gl.glViewport(0, 0, width, height);

        switch (target) {
            case GL_TEXTURE_2D:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                D("with glFramebufferTexture2D + glReadPixels");
                gl.glFramebufferTexture2D(
                        fbTarget, attachment, target,
                        globalName, level);
                gl.glReadPixels(0, 0, width, height,
                        format, type,
                        data);
                gl.glFramebufferTexture2D(
                        fbTarget, attachment, target,
                        0, level);
                break;
            case GL_TEXTURE_3D: {
                D("with glFramebufferTexture3DOES + glReadPixels");
                unsigned int layerImgSize = s_texImageSize(
                        format, type, 1, width, height);
                for (unsigned int d = 0; d < depth; d++) {
                    gl.glFramebufferTexture3DOES(
                            fbTarget, attachment, target,
                            globalName, level, d);
                    gl.glReadPixels(0, 0, width,
                            height, format,
                            type, data +
                            layerImgSize * d);
                    gl.glFramebufferTexture3DOES(
                            fbTarget, attachment, target,
                            0, level, d);
                }
                break;
            }
            case GL_TEXTURE_2D_ARRAY: {
                D("with glFramebufferTextureLayer + glReadPixels");
                unsigned int layerImgSize = s_texImageSize(
                        format, type, 1, width, height);
                for (unsigned int d = 0; d < depth; d++) {
                    gl.glFramebufferTextureLayer(
                            fbTarget, attachment,
                            globalName, level, d);
                    gl.glReadPixels(0, 0, width,
                            height, format,
                            type, data +
                            layerImgSize * d);
                    gl.glFramebufferTextureLayer(
                            fbTarget, attachment,
                            0, level, d);
                }
                break;
            }
        }
    }

    void preSave() {
        setupFbo();
    }

    void postSave() {
        teardownFbo();
    }
};

static LazyInstance<TextureDataReader> sTextureDataReader = LAZY_INSTANCE_INIT;

void SaveableTexture::preSave() {
    sTextureDataReader->preSave();
}

void SaveableTexture::postSave() {
    sTextureDataReader->postSave();
}

SaveableTexture::SaveableTexture(const TextureData& texture)
    : m_target(texture.target),
      m_width(texture.width),
      m_height(texture.height),
      m_depth(texture.depth),
      m_format(texture.format),
      m_internalFormat(texture.internalFormat),
      m_type(texture.type),
      m_border(texture.border),
      m_texStorageLevels(texture.texStorageLevels),
      m_globalName(texture.getGlobalName()),
      m_isDirty(true) {}

SaveableTexture::SaveableTexture(GlobalNameSpace* globalNameSpace,
                                 loader_t&& loader)
    : m_loader(std::move(loader)),
      m_globalNamespace(globalNameSpace),
      m_isDirty(false) {
    mNeedRestore = true;
}

void SaveableTexture::loadFromStream(android::base::Stream* stream) {
    m_target = stream->getBe32();
    m_width = stream->getBe32();
    m_height = stream->getBe32();
    m_depth = stream->getBe32();
    m_format = stream->getBe32();
    m_internalFormat = stream->getBe32();
    m_type = stream->getBe32();
    m_border = stream->getBe32();
    m_texStorageLevels = stream->getBe32();
    m_maxMipmapLevel = stream->getBe32();
    // TODO: handle other texture targets
    if (m_target == GL_TEXTURE_2D || m_target == GL_TEXTURE_CUBE_MAP ||
        m_target == GL_TEXTURE_3D || m_target == GL_TEXTURE_2D_ARRAY) {
        unsigned int numLevels = m_texStorageLevels ? m_texStorageLevels :
                m_maxMipmapLevel + 1;
        auto loadTex = [stream, numLevels](
                               std::unique_ptr<LevelImageData[]>& levelData,
                               bool isDepth) {
            levelData.reset(new LevelImageData[numLevels]);
            for (unsigned int level = 0; level < numLevels; level++) {
                levelData[level].m_width = stream->getBe32();
                levelData[level].m_height = stream->getBe32();
                if (isDepth) {
                    levelData[level].m_depth = stream->getBe32();
                }
                loadBuffer(stream, &levelData[level].m_data);
            }
        };
        switch (m_target) {
            case GL_TEXTURE_2D:
                loadTex(m_levelData[0], false);
                break;
            case GL_TEXTURE_CUBE_MAP:
                for (int i = 0; i < 6; i++) {
                    loadTex(m_levelData[i], false);
                }
                break;
            case GL_TEXTURE_3D:
            case GL_TEXTURE_2D_ARRAY:
                loadTex(m_levelData[0], true);
                break;
            default:
                break;
        }
        // Load tex param
        loadCollection(stream, &m_texParam,
                [](android::base::Stream* stream)
                    -> std::unordered_map<GLenum, GLint>::value_type {
                    GLenum pname = stream->getBe32();
                    GLint value = stream->getBe32();
                    return std::make_pair(pname, value);
                });
    } else if (m_target != 0) {
        GL_LOG("SaveableTexture::%s: warning: texture target 0x%x not "
               "supported\n",
               __func__, m_target);
        fprintf(stderr, "Warning: texture target %d not supported\n", m_target);
    }
    m_loadedFromStream.store(true);
}

void SaveableTexture::onSave(
        android::base::Stream* stream) {
    stream->putBe32(m_target);
    stream->putBe32(m_width);
    stream->putBe32(m_height);
    stream->putBe32(m_depth);
    stream->putBe32(m_format);
    stream->putBe32(m_internalFormat);
    stream->putBe32(m_type);
    stream->putBe32(m_border);
    stream->putBe32(m_texStorageLevels);
    stream->putBe32(m_maxMipmapLevel);
    // TODO: handle other texture targets
    if (m_target == GL_TEXTURE_2D || m_target == GL_TEXTURE_CUBE_MAP ||
        m_target == GL_TEXTURE_3D || m_target == GL_TEXTURE_2D_ARRAY) {
        static constexpr GLenum pixelStoreIndexes[] = {
                GL_PACK_ROW_LENGTH, GL_PACK_SKIP_PIXELS, GL_PACK_SKIP_ROWS,
                GL_PACK_ALIGNMENT,
        };
        static constexpr GLint pixelStoreDesired[] = {0, 0, 0, 1};
        GLint pixelStorePrev[android::base::arraySize(pixelStoreIndexes)];

        GLint prevTex = 0;
        GLDispatch& dispatcher = GLEScontext::dispatcher();
        assert(dispatcher.glGetIntegerv);
        for (int i = 0; i != android::base::arraySize(pixelStoreIndexes); ++i) {
            if (isGles2Gles() && pixelStoreIndexes[i] != GL_PACK_ALIGNMENT &&
                pixelStoreIndexes[i] != GL_UNPACK_ALIGNMENT) {
                continue;
            }
            dispatcher.glGetIntegerv(pixelStoreIndexes[i], &pixelStorePrev[i]);
            if (pixelStorePrev[i] != pixelStoreDesired[i]) {
                dispatcher.glPixelStorei(pixelStoreIndexes[i],
                                         pixelStoreDesired[i]);
            }
        }
        switch (m_target) {
            case GL_TEXTURE_2D:
                dispatcher.glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
                break;
            case GL_TEXTURE_CUBE_MAP:
                dispatcher.glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &prevTex);
                break;
            case GL_TEXTURE_3D:
                dispatcher.glGetIntegerv(GL_TEXTURE_BINDING_3D, &prevTex);
                break;
            case GL_TEXTURE_2D_ARRAY:
                dispatcher.glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &prevTex);
                break;
            default:
                break;
        }

        dispatcher.glBindTexture(m_target, getGlobalName());
        // Get the number of mipmap levels.
        unsigned int numLevels = m_texStorageLevels ? m_texStorageLevels :
                m_maxMipmapLevel + 1;

        // bug: 112749908
        // Texture saving causes hundreds of megabytes of memory ballooning.
        // This could be behind nullptr dereferences in crash reports if
        // the user ran out of commit charge on Windows, which is not measured
        // in android::base::System::isUnderMemoryPressure.
        //
        // To debug this issue, avoid keeping the imgData buffers around,
        // and log the memory usage.
        //
        // bool isLowMem = android::base::System::isUnderMemoryPressure();
        bool isLowMem = true;

        auto saveTex = [this, stream, numLevels, &dispatcher, isLowMem](
                                GLenum target, bool isDepth,
                                std::unique_ptr<LevelImageData[]>& imgData) {

            if (m_isDirty) {
                imgData.reset(new LevelImageData[numLevels]);
                for (unsigned int level = 0; level < numLevels; level++) {
                    unsigned int& width = imgData.get()[level].m_width;
                    unsigned int& height = imgData.get()[level].m_height;
                    unsigned int& depth = imgData.get()[level].m_depth;
                    width = level == 0 ? m_width :
                        std::max<unsigned int>(
                            imgData.get()[level - 1].m_width / 2, 1);
                    height = level == 0 ? m_height :
                        std::max<unsigned int>(
                            imgData.get()[level - 1].m_height / 2, 1);
                    depth = level == 0 ? m_depth :
                        std::max<unsigned int>(
                            imgData.get()[level - 1].m_depth / 2, 1);

                    ScopedMemoryProfiler::Callback memoryProfilerCallback =
                        [this, level, width, height, depth]
                        (StringView tag, StringView stage,
                         MemoryProfiler::MemoryUsageBytes currentResident,
                         MemoryProfiler::MemoryUsageBytes change) {

                        double megabyte = 1024.0 * 1024.0;

                        GL_LOG("%s %s: %f mb current. change: %f mb. texture:"
                               "format 0x%x type 0x%x level 0x%x dims (%u, %u, %u)\n",
                               c_str(tag).get(),
                               c_str(stage).get(),
                               (double)currentResident / megabyte,
                               (double)change / megabyte,
                               m_format,
                               m_type,
                               level,
                               width, height, depth);
                    };

                    ScopedMemoryProfiler mem("saveTexture", memoryProfilerCallback);

                    android::base::SmallFixedVector<unsigned char, 16>& buffer
                        = imgData.get()[level].m_data;
                    if (!isGles2Gles()) {
                        GLint glWidth;
                        GLint glHeight;
                        dispatcher.glGetTexLevelParameteriv(target, level,
                                GL_TEXTURE_WIDTH, &glWidth);
                        dispatcher.glGetTexLevelParameteriv(target, level,
                                GL_TEXTURE_HEIGHT, &glHeight);
                        width = static_cast<unsigned int>(glWidth);
                        height = static_cast<unsigned int>(glHeight);
                    }
                    if (isDepth) {
                        if (!isGles2Gles()) {
                            GLint glDepth;
                            dispatcher.glGetTexLevelParameteriv(target, level,
                                    GL_TEXTURE_DEPTH, &glDepth);
                            depth = static_cast<unsigned int>(std::max(glDepth,
                                    1));
                        }
                    } else {
                        depth = 1;
                    }
                    // Snapshot texture data
                    buffer.clear();
                    buffer.resize_noinit(
                            s_texImageSize(m_format, m_type, 1, width, height) *
                            depth);
                    if (!buffer.empty()) {
                        GLenum neededBufferFormat = m_format;
                        if (isCoreProfile()) {
                            neededBufferFormat =
                                getCoreProfileEmulatedFormat(m_format);
                        }
                        sTextureDataReader->getTexImage(
                            m_globalName, target, level, neededBufferFormat, m_type, width, height, depth, buffer.data());
                    }
                }
            }
            for (unsigned int level = 0; level < numLevels; level++) {
                stream->putBe32(imgData.get()[level].m_width);
                stream->putBe32(imgData.get()[level].m_height);
                if (isDepth) {
                    stream->putBe32(imgData.get()[level].m_depth);
                }
                saveBuffer(stream, imgData.get()[level].m_data);
            }

            // If under memory pressure, delete this intermediate buffer.
            if (isLowMem) {
                imgData.reset();
            }
        };
        switch (m_target) {
            case GL_TEXTURE_2D:
                saveTex(GL_TEXTURE_2D, false, m_levelData[0]);
                break;
            case GL_TEXTURE_CUBE_MAP:
                saveTex(GL_TEXTURE_CUBE_MAP_POSITIVE_X, false, m_levelData[0]);
                saveTex(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, false, m_levelData[1]);
                saveTex(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, false, m_levelData[2]);
                saveTex(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, false, m_levelData[3]);
                saveTex(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, false, m_levelData[4]);
                saveTex(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, false, m_levelData[5]);
                break;
            case GL_TEXTURE_3D:
                saveTex(GL_TEXTURE_3D, true, m_levelData[0]);
                break;
            case GL_TEXTURE_2D_ARRAY:
                saveTex(GL_TEXTURE_2D_ARRAY, true, m_levelData[0]);
                break;
            default:
                break;
        }
        // Snapshot texture param
        TextureSwizzle emulatedBaseSwizzle;
        if (isCoreProfile()) {
            emulatedBaseSwizzle = getSwizzleForEmulatedFormat(m_format);
        }
        std::unordered_map<GLenum, GLint> texParam;
        auto saveParam = [this, &texParam, stream, &dispatcher,
                emulatedBaseSwizzle](
                const GLenum* plist, size_t plistSize) {
            GLint param;
            for (size_t i = 0; i < plistSize; i++) {
                dispatcher.glGetTexParameteriv(m_target, plist[i], &param);
                if (isSwizzleParam(plist[i]) && param != GL_ZERO &&
                        param != GL_ONE) {
                    if (param == emulatedBaseSwizzle.toRed) {
                        param = GL_RED;
                    } else if (param == emulatedBaseSwizzle.toGreen) {
                        param = GL_GREEN;
                    } else if (param == emulatedBaseSwizzle.toBlue) {
                        param = GL_BLUE;
                    } else if (param == emulatedBaseSwizzle.toAlpha) {
                        param = GL_ALPHA;
                    }
                }
                texParam.emplace(plist[i], param);
            }
        };
        saveParam(kTexParam, sizeof(kTexParam) / sizeof(kTexParam[0]));
        if (dispatcher.getGLESVersion() >= GLES_3_0) {
            saveParam(kTexParamGles3,
                    sizeof(kTexParamGles3) / sizeof(kTexParamGles3[0]));
        }
        saveCollection(stream, texParam,
                [](android::base::Stream* s,
                    const std::unordered_map<GLenum, GLint>::value_type& pair) {
                    s->putBe32(pair.first);
                    s->putBe32(pair.second);
                });
        // Restore environment
        for (int i = 0; i != android::base::arraySize(pixelStoreIndexes); ++i) {
            if (isGles2Gles() && pixelStoreIndexes[i] != GL_PACK_ALIGNMENT &&
                pixelStoreIndexes[i] != GL_UNPACK_ALIGNMENT) {
                continue;
            }
            if (pixelStorePrev[i] != pixelStoreDesired[i]) {
                dispatcher.glPixelStorei(pixelStoreIndexes[i],
                                         pixelStorePrev[i]);
            }
        }
        dispatcher.glBindTexture(m_target, prevTex);

        // If we were under memory pressure, we deleted the intermediate
        // buffer, so we need to maintain the invariant that m_isDirty = false
        // textures requires that the intermediate buffer is still around.
        // Therefore, mark as dirty if we were under memory pressure.
        //
        // TODO: Don't keep those around in memory regardless of memory
        // pressure

        m_isDirty = false || isLowMem;
    } else if (m_target != 0) {
        // SaveableTexture is uninitialized iff a texture hasn't been bound,
        // which will give m_target==0
        GL_LOG("SaveableTexture::%s: warning: texture target 0x%x not supported\n", m_target);
        fprintf(stderr, "Warning: texture target 0x%x not supported\n", m_target);
    }
}

void SaveableTexture::restore() {
    assert(m_loader);
    m_loader(this);

    if (!m_loadedFromStream.load()) {
        return;
    }

    m_globalTexObj.reset(new NamedObject(
            GenNameInfo(NamedObjectType::TEXTURE), m_globalNamespace));
    if (!m_globalTexObj) {
        GL_LOG("SaveableTexture::%s: %p: could not allocate NamedObject for texture\n", __func__, this);
        emugl::emugl_crash_reporter(
                "Fatal: could not allocate SaveableTexture m_globalTexObj\n");
    }

    m_globalName = m_globalTexObj->getGlobalName();
    if (m_target == GL_TEXTURE_2D || m_target == GL_TEXTURE_CUBE_MAP ||
        m_target == GL_TEXTURE_3D || m_target == GL_TEXTURE_2D_ARRAY) {
        // restore the texture
        GLDispatch& dispatcher = GLEScontext::dispatcher();
        // Make sure we are using the right dispatcher
        assert(dispatcher.glGetIntegerv);

        static constexpr GLenum pixelStoreIndexes[] = {
                GL_UNPACK_ROW_LENGTH,  GL_UNPACK_IMAGE_HEIGHT,
                GL_UNPACK_SKIP_PIXELS, GL_UNPACK_SKIP_ROWS,
                GL_UNPACK_SKIP_IMAGES, GL_UNPACK_ALIGNMENT,
        };

        static constexpr GLint pixelStoreDesired[] = {0, 0, 0, 0, 0, 1};

        GLint pixelStorePrev[android::base::arraySize(pixelStoreIndexes)];
        for (int i = 0; i != android::base::arraySize(pixelStoreIndexes); ++i) {
            if (isGles2Gles() && pixelStoreIndexes[i] != GL_PACK_ALIGNMENT &&
                pixelStoreIndexes[i] != GL_UNPACK_ALIGNMENT) {
                continue;
            }
            dispatcher.glGetIntegerv(pixelStoreIndexes[i], &pixelStorePrev[i]);
            if (pixelStorePrev[i] != pixelStoreDesired[i]) {
                dispatcher.glPixelStorei(pixelStoreIndexes[i],
                                         pixelStoreDesired[i]);
            }
        }

        GLint prevTex = 0;
        switch (m_target) {
            case GL_TEXTURE_2D:
                dispatcher.glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
                break;
            case GL_TEXTURE_CUBE_MAP:
                dispatcher.glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &prevTex);
                break;
            case GL_TEXTURE_3D:
                dispatcher.glGetIntegerv(GL_TEXTURE_BINDING_3D, &prevTex);
                break;
            case GL_TEXTURE_2D_ARRAY:
                dispatcher.glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &prevTex);
                break;
            default:
                break;
        }
        dispatcher.glBindTexture(m_target, getGlobalName());
        // Restore texture data
        dispatcher.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        // Get the number of mipmap levels.
        unsigned int numLevels = m_texStorageLevels ? m_texStorageLevels :
                m_maxMipmapLevel + 1;
        GLint resultInternalFormat = m_internalFormat;
        GLenum resultFormat = m_format;
        // Desktop OpenGL doesn't support GL_BGRA_EXT as internal format.
        if (!isGles2Gles() && m_type == GL_UNSIGNED_BYTE && m_format == GL_BGRA_EXT &&
            resultInternalFormat == GL_BGRA_EXT) {
            resultInternalFormat = GL_RGBA;
        } else if (isCoreProfile() && isCoreProfileEmulatedFormat(m_format)) {
            resultInternalFormat = getCoreProfileEmulatedInternalFormat(
                    m_format, m_type);
            resultFormat = getCoreProfileEmulatedFormat(m_format);
        }
        if (m_texStorageLevels) {
            switch (m_target) {
                case GL_TEXTURE_2D:
                case GL_TEXTURE_CUBE_MAP:
                    dispatcher.glTexStorage2D(m_target, m_texStorageLevels,
                            m_internalFormat,
                            m_levelData[0].get()[0].m_width,
                            m_levelData[0].get()[0].m_height);
                    break;
                case GL_TEXTURE_3D:
                case GL_TEXTURE_2D_ARRAY:
                    dispatcher.glTexStorage3D(m_target, m_texStorageLevels,
                            m_internalFormat,
                            m_levelData[0].get()[0].m_width,
                            m_levelData[0].get()[0].m_height,
                            m_levelData[0].get()[0].m_depth);
                    break;
            }
        }

        auto restoreTex2D =
                [this, numLevels, resultInternalFormat,
                resultFormat, &dispatcher](
                        GLenum target,
                        std::unique_ptr<LevelImageData[]>& levelData) {
                    for (unsigned int level = 0; level < numLevels; level++) {
                        const void* pixels =
                                levelData[level].m_data.empty()
                                        ? nullptr
                                        : levelData[level].m_data.data();
                        if (!level || pixels) {
                            if (m_texStorageLevels) {
                                dispatcher.glTexSubImage2D(
                                        target, level, 0, 0,
                                        levelData[level].m_width,
                                        levelData[level].m_height,
                                        resultFormat, m_type, pixels);
                            } else {
                                dispatcher.glTexImage2D(
                                        target, level, resultInternalFormat,
                                        levelData[level].m_width,
                                        levelData[level].m_height,
                                        m_border, resultFormat, m_type, pixels);
                            }
                        }
                    }
                };
        auto restoreTex3D =
                [this, numLevels, resultInternalFormat,
                resultFormat, &dispatcher](
                        GLenum target,
                        std::unique_ptr<LevelImageData[]>& levelData) {
                    for (unsigned int level = 0; level < numLevels; level++) {
                        const void* pixels =
                                levelData[level].m_data.empty()
                                        ? nullptr
                                        : levelData[level].m_data.data();
                        if (!level || pixels) {
                            if (m_texStorageLevels) {
                                dispatcher.glTexSubImage3D(
                                        target, level, 0, 0, 0,
                                        levelData[level].m_width,
                                        levelData[level].m_height,
                                        levelData[level].m_depth,
                                        resultFormat, m_type, pixels);
                            } else {
                                dispatcher.glTexImage3D(
                                        target, level, m_internalFormat,
                                        levelData[level].m_width,
                                        levelData[level].m_height,
                                        levelData[level].m_depth, m_border,
                                        resultFormat, m_type, pixels);
                            }
                        }
                    }
                };
        switch (m_target) {
            case GL_TEXTURE_2D:
                restoreTex2D(GL_TEXTURE_2D, m_levelData[0]);
                break;
            case GL_TEXTURE_CUBE_MAP:
                restoreTex2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, m_levelData[0]);
                restoreTex2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, m_levelData[1]);
                restoreTex2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, m_levelData[2]);
                restoreTex2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, m_levelData[3]);
                restoreTex2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, m_levelData[4]);
                restoreTex2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, m_levelData[5]);
                break;
            case GL_TEXTURE_3D:
            case GL_TEXTURE_2D_ARRAY:
                restoreTex3D(m_target, m_levelData[0]);
                break;
            default:
                break;
        }
        // Restore tex param
        TextureSwizzle emulatedBaseSwizzle;
        if (isCoreProfile()) {
            emulatedBaseSwizzle = getSwizzleForEmulatedFormat(m_format);
        }
        for (const auto& param : m_texParam) {
            if (isSwizzleParam(param.first)) {
                GLenum hostEquivalentSwizzle =
                    swizzleComponentOf(emulatedBaseSwizzle, param.second);
                dispatcher.glTexParameteri(m_target, param.first,
                        hostEquivalentSwizzle);
            } else {
                dispatcher.glTexParameteri(m_target, param.first, param.second);
            }
        }
        m_texParam.clear();
        // Restore environment
        for (int i = 0; i != android::base::arraySize(pixelStoreIndexes); ++i) {
            if (isGles2Gles() && pixelStoreIndexes[i] != GL_PACK_ALIGNMENT &&
                pixelStoreIndexes[i] != GL_UNPACK_ALIGNMENT) {
                continue;
            }
            if (pixelStorePrev[i] != pixelStoreDesired[i]) {
                dispatcher.glPixelStorei(pixelStoreIndexes[i],
                                         pixelStorePrev[i]);
            }
        }
        dispatcher.glBindTexture(m_target, prevTex);
    }
}

const NamedObjectPtr& SaveableTexture::getGlobalObject() {
    touch();
    return m_globalTexObj;
}

void SaveableTexture::fillEglImage(EglImage* eglImage) {
    touch();
    eglImage->border = m_border;
    eglImage->format = m_format;
    eglImage->height = m_height;
    eglImage->globalTexObj = m_globalTexObj;
    eglImage->internalFormat = m_internalFormat;
    eglImage->type = m_type;
    eglImage->width = m_width;
    eglImage->texStorageLevels = m_texStorageLevels;
    eglImage->sync = nullptr;
    if (!eglImage->globalTexObj) {
        GL_LOG("%s: EGL image %p has no global texture object!\n",
               __func__, eglImage);
    }
}

void SaveableTexture::makeDirty() {
    m_isDirty = true;
}

bool SaveableTexture::isDirty() const {
    return m_isDirty;
}

void SaveableTexture::setTarget(GLenum target) {
    m_target = target;
}

void SaveableTexture::setMipmapLevelAtLeast(unsigned int level) {
    m_maxMipmapLevel = std::max(level, m_maxMipmapLevel);
}

unsigned int SaveableTexture::getGlobalName() {
    if (m_globalTexObj) {
        return m_globalTexObj->getGlobalName();
    }
    return m_globalName;
}
