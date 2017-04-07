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

#include "android/base/files/StreamSerializing.h"
#include "GLcommon/GLEScontext.h"

#include <algorithm>

static uint32_t s_texAlign(uint32_t v, uint32_t align) {

    uint32_t rem = v % align;
    return rem ? (v + (align - rem)) : v;

}

static uint32_t s_texPixelSize(GLenum internalformat,
                              GLenum type) {
    uint32_t reps = 3;
    switch (internalformat) {
    case GL_ALPHA:
        reps = 1;
        break;
    case GL_RGB:
        if (type == GL_UNSIGNED_SHORT_5_6_5)
            reps = 1;
        else
            reps = 3;
        break;
    case GL_DEPTH_STENCIL:
        if (type == GL_FLOAT_32_UNSIGNED_INT_24_8_REV) {
            reps = 8;
        } else if (type == GL_UNSIGNED_INT_24_8) {
            reps = 4;
        } else {
            assert(!"Invalid type for GL_DEPTH_STENCIL texture");
        }
        break;
    case GL_RGBA:
        reps = 4;
        break;
    default:
        break;
    }

    uint32_t eltSize = 1;

    switch (type) {
    case GL_UNSIGNED_SHORT_5_6_5:
        eltSize = 2;
        break;
    case GL_UNSIGNED_BYTE:
        eltSize = 1;
        break;
    default:
        break;
    }

    uint32_t pixelSize = reps * eltSize;

    return pixelSize;
}

static uint32_t s_texImageSize(GLenum internalformat,
                              GLenum type,
                              int unpackAlignment,
                              GLsizei width, GLsizei height) {

    uint32_t alignedWidth = s_texAlign(width, unpackAlignment);
    uint32_t pixelSize = s_texPixelSize(internalformat, type);
    uint32_t totalSize = pixelSize * alignedWidth * height;

    return totalSize;
}

SaveableTexture::SaveableTexture(const EglImage& eglImage)
    : m_width(eglImage.width)
    , m_height(eglImage.height)
    , m_format(eglImage.format)
    , m_internalFormat(eglImage.internalFormat)
    , m_type(eglImage.type)
    , m_border(eglImage.border)
    , m_globalName(eglImage.globalTexObj->getGlobalName()) { }

SaveableTexture::SaveableTexture(const TextureData& texture)
    : m_target(texture.target)
    , m_width(texture.width)
    , m_height(texture.height)
    , m_depth(texture.depth)
    , m_format(texture.format)
    , m_internalFormat(texture.internalFormat)
    , m_type(texture.type)
    , m_border(texture.border)
    , m_globalName(texture.globalName)
    { }

SaveableTexture::SaveableTexture(android::base::Stream* stream,
        GlobalNameSpace* globalNameSpace)
    : m_globalTexObj(new NamedObject(GenNameInfo(NamedObjectType::TEXTURE),
        globalNameSpace)) {
    m_target = stream->getBe32();
    m_width = stream->getBe32();
    m_height = stream->getBe32();
    m_depth = stream->getBe32();
    m_format = stream->getBe32();
    m_internalFormat = stream->getBe32();
    m_type = stream->getBe32();
    m_border = stream->getBe32();
    // TODO: handle other texture targets
    if (m_target == GL_TEXTURE_2D || m_target == GL_TEXTURE_CUBE_MAP
            || m_target == GL_TEXTURE_3D || m_target == GL_TEXTURE_2D_ARRAY) {
        // restore the texture
        // TODO: move this to higher level for performance
        std::unordered_map<GLenum, GLint> desiredPixelStori = {
            {GL_UNPACK_ROW_LENGTH, 0},
            {GL_UNPACK_IMAGE_HEIGHT, 0},
            {GL_UNPACK_SKIP_PIXELS, 0},
            {GL_UNPACK_SKIP_ROWS, 0},
            {GL_UNPACK_SKIP_IMAGES, 0},
            {GL_UNPACK_ALIGNMENT, 1},
        };
        std::unordered_map<GLenum, GLint> prevPixelStori = desiredPixelStori;
        GLDispatch& dispatcher = GLEScontext::dispatcher();
        for (auto& ps : prevPixelStori) {
            dispatcher.glGetIntegerv(ps.first, &ps.second);
        }
        for (auto& ps : desiredPixelStori) {
            dispatcher.glPixelStorei(ps.first, ps.second);
        }
        m_globalName = m_globalTexObj->getGlobalName();
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
        dispatcher.glBindTexture(m_target, m_globalName);
        // Restore texture data
        dispatcher.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        // Get the number of mipmap levels.
        unsigned int numLevels = 1 + floor(log2(
                (float)std::max(m_width, m_height)));
        auto restoreTex2D = [stream, internalFormat=m_internalFormat,
                border=m_border, format=m_format, type=m_type, numLevels,
                &dispatcher] (GLenum target) {
                for (unsigned int level = 0; level < numLevels; level++) {
                    GLint width = stream->getBe32();
                    GLint height = stream->getBe32();
                    std::vector<unsigned char> loadedData;
                    loadBuffer(stream, &loadedData);
                    const void* pixels = loadedData.empty() ? nullptr :
                                                              loadedData.data();
                    dispatcher.glTexImage2D(target, level, internalFormat,
                            width, height, border, format, type, pixels);
                }
        };
        auto restoreTex3D = [stream, internalFormat=m_internalFormat,
                border=m_border, format=m_format, type=m_type, numLevels,
                &dispatcher] (GLenum target) {
                for (unsigned int level = 0; level < numLevels; level++) {
                    GLint width = stream->getBe32();
                    GLint height = stream->getBe32();
                    GLint depth = stream->getBe32();
                    std::vector<unsigned char> loadedData;
                    loadBuffer(stream, &loadedData);
                    const void* pixels = loadedData.empty() ? nullptr :
                                                              loadedData.data();
                    dispatcher.glTexImage3D(target, level, internalFormat,
                            width, height, depth, border, format, type, pixels);
                }
        };
        switch (m_target) {
            case GL_TEXTURE_2D:
                restoreTex2D(GL_TEXTURE_2D);
                break;
            case GL_TEXTURE_CUBE_MAP:
                restoreTex2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X);
                restoreTex2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
                restoreTex2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
                restoreTex2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
                restoreTex2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
                restoreTex2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
                break;
            case GL_TEXTURE_3D:
            case GL_TEXTURE_2D_ARRAY:
                restoreTex3D(GL_TEXTURE_3D);
                break;
            default:
                break;

        }
        // Restore tex param
        auto loadParam = [stream, target=m_target, &dispatcher](GLenum pname) {
            GLint param = stream->getBe32();
            dispatcher.glTexParameteri(target, pname, param);
        };
        loadParam(GL_TEXTURE_MAG_FILTER);
        loadParam(GL_TEXTURE_MIN_FILTER);
        loadParam(GL_TEXTURE_WRAP_S);
        loadParam(GL_TEXTURE_WRAP_T);
        // Restore environment
        for (auto& ps : prevPixelStori) {
            dispatcher.glPixelStorei(ps.first, ps.second);
        }
        dispatcher.glBindTexture(m_target, prevTex);
    } else {
        fprintf(stderr, "Warning: texture target %d not supported\n", m_target);
    }
}

void SaveableTexture::onSave(android::base::Stream* stream) const {
    stream->putBe32(m_target);
    stream->putBe32(m_width);
    stream->putBe32(m_height);
    stream->putBe32(m_depth);
    stream->putBe32(m_format);
    stream->putBe32(m_internalFormat);
    stream->putBe32(m_type);
    stream->putBe32(m_border);
    // TODO: handle other texture targets
    if (m_target == GL_TEXTURE_2D || m_target == GL_TEXTURE_CUBE_MAP
            || m_target == GL_TEXTURE_3D || m_target == GL_TEXTURE_2D_ARRAY) {
        // TODO: move this to higher level for performance
        std::unordered_map<GLenum, GLint> desiredPixelStori = {
            {GL_PACK_ROW_LENGTH, 0},
            {GL_PACK_SKIP_PIXELS, 0},
            {GL_PACK_SKIP_ROWS, 0},
            {GL_PACK_ALIGNMENT, 1},
        };
        std::unordered_map<GLenum, GLint> prevPixelStori = desiredPixelStori;
        GLint prevTex = 0;
        GLDispatch& dispatcher = GLEScontext::dispatcher();
        assert(dispatcher.glGetIntegerv);
        for (auto& ps : prevPixelStori) {
            dispatcher.glGetIntegerv(ps.first, &ps.second);
        }
        for (auto& ps : desiredPixelStori) {
            dispatcher.glPixelStorei(ps.first, ps.second);
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
        dispatcher.glBindTexture(m_target, m_globalName);
        // Get the number of mipmap levels.
        unsigned int numLevels = 1 + floor(log2(
                (float)std::max(m_width, m_height)));
        auto saveTex2D = [stream, format=m_format, type=m_type, numLevels,
                &dispatcher] (GLenum target) {
            for (unsigned int level = 0; level < numLevels; level++) {
                GLint width = 1;
                GLint height = 1;
                dispatcher.glGetTexLevelParameteriv(target, level,
                        GL_TEXTURE_WIDTH, &width);
                dispatcher.glGetTexLevelParameteriv(target, level,
                        GL_TEXTURE_HEIGHT, &height);
                stream->putBe32(width);
                stream->putBe32(height);
                // Snapshot texture data
                std::vector<unsigned char> tmpData(s_texImageSize(format,
                        type, 1, width, height));
                dispatcher.glGetTexImage(target, level, format, type,
                        tmpData.data());
                saveBuffer(stream, tmpData);
            }
        };
        auto saveTex3D = [stream, format=m_format, type=m_type, numLevels,
                &dispatcher] (GLenum target) {
            for (unsigned int level = 0; level < numLevels; level++) {
                GLint width = 1;
                GLint height = 1;
                GLint depth = 1;
                dispatcher.glGetTexLevelParameteriv(target, level,
                        GL_TEXTURE_WIDTH, &width);
                dispatcher.glGetTexLevelParameteriv(target, level,
                        GL_TEXTURE_HEIGHT, &height);
                dispatcher.glGetTexLevelParameteriv(target, level,
                        GL_TEXTURE_DEPTH, &depth);
                stream->putBe32(width);
                stream->putBe32(height);
                stream->putBe32(depth);
                // Snapshot texture data
                std::vector<unsigned char> tmpData(s_texImageSize(format,
                        type, 1, width, height) * depth);
                dispatcher.glGetTexImage(target, level, format, type,
                        tmpData.data());
                saveBuffer(stream, tmpData);
            }
        };
        switch (m_target) {
            case GL_TEXTURE_2D:
                saveTex2D(GL_TEXTURE_2D);
                break;
            case GL_TEXTURE_CUBE_MAP:
                saveTex2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X);
                saveTex2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
                saveTex2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
                saveTex2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
                saveTex2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
                saveTex2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
                break;
            case GL_TEXTURE_3D:
            case GL_TEXTURE_2D_ARRAY:
                saveTex3D(GL_TEXTURE_3D);
                break;
            default:
                break;
        }
        // Snapshot texture param
        auto saveParam = [stream, target=m_target, &dispatcher](GLenum pname) {
            GLint param;
            dispatcher.glGetTexParameteriv(target, pname, &param);
            stream->putBe32(param);
        };
        saveParam(GL_TEXTURE_MAG_FILTER);
        saveParam(GL_TEXTURE_MIN_FILTER);
        saveParam(GL_TEXTURE_WRAP_S);
        saveParam(GL_TEXTURE_WRAP_T);
        // Restore environment
        for (auto& ps : prevPixelStori) {
            dispatcher.glPixelStorei(ps.first, ps.second);
        }
        dispatcher.glBindTexture(m_target, prevTex);
    } else {
        fprintf(stderr, "Warning: texture target 0x%x not supported\n", m_target);
    }
}

NamedObjectPtr SaveableTexture::getGlobalObject() const {
    return m_globalTexObj;
}

EglImage* SaveableTexture::makeEglImage() const {
    EglImage* ret = new EglImage();
    ret->border = m_border;
    ret->format = m_format;
    ret->height = m_height;
    ret->globalTexObj = m_globalTexObj;
    ret->internalFormat = m_internalFormat;
    ret->type = m_type;
    ret->width = m_width;
    return ret;
}
