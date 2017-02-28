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
#include "GLcommon/GLDispatch.h"

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
    , m_border(eglImage.border) { }

SaveableTexture::SaveableTexture(const TextureData& texture)
    : m_target(texture.target)
    , m_width(texture.width)
    , m_height(texture.height)
    , m_format(texture.format)
    , m_internalFormat(texture.internalFormat)
    , m_type(texture.type)
    , m_border(texture.border)
    { }

SaveableTexture::SaveableTexture(android::base::Stream* stream,
        GlobalNameSpace* globalNameSpace, GLDispatch* dispatcher)
    : m_globalTexObj(new NamedObject(GenNameInfo(NamedObjectType::TEXTURE),
        globalNameSpace)) {
    m_target = stream->getBe32();
    m_width = stream->getBe32();
    m_height = stream->getBe32();
    m_format = stream->getBe32();
    m_type = stream->getBe32();
    m_border = stream->getBe32();
    // TODO: handle other texture targets
    if (m_target == GL_TEXTURE_2D) {
        std::vector<unsigned char> loadedData;
        loadBuffer(stream, &loadedData);
        // restore the texture
        unsigned int globalName = m_globalTexObj->getGlobalName();
        dispatcher->glBindTexture(m_target, globalName);
        // TODO: snapshot mipmaps
        GLint prevUnpack = 4;
        dispatcher->glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevUnpack);
        dispatcher->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        const void* pixels = loadedData.empty() ? nullptr : loadedData.data();
        dispatcher->glTexImage2D(m_target, 0, m_internalFormat, m_width,
                m_height, m_border, m_format, m_type, pixels);
        // TODO: restore tex param
        dispatcher->glPixelStorei(GL_UNPACK_ALIGNMENT, prevUnpack);
    }
}

void SaveableTexture::onSave(android::base::Stream* stream,
        unsigned int globalName, GLDispatch* dispatcher) const {
    stream->putBe32(m_target);
    stream->putBe32(m_width);
    stream->putBe32(m_height);
    stream->putBe32(m_format);
    stream->putBe32(m_type);
    stream->putBe32(m_border);
    // TODO: handle other texture targets
    if (m_target == GL_TEXTURE_2D) {
        GLint prevPack = 4;
        GLint prevTex = 0;
        dispatcher->glGetIntegerv(GL_PACK_ALIGNMENT, &prevPack);
        dispatcher->glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
        dispatcher->glPixelStorei(GL_PACK_ALIGNMENT, 1);
        dispatcher->glBindTexture(GL_TEXTURE_2D, globalName);
        std::vector<unsigned char> tmpData(s_texImageSize(m_format, m_type, 1,
                m_width, m_height));
        dispatcher->glGetTexImage(m_target, 0, m_format, m_type, tmpData.data());
        dispatcher->glPixelStorei(GL_PACK_ALIGNMENT, prevPack);
        dispatcher->glBindTexture(GL_TEXTURE_2D, prevTex);
        saveBuffer(stream, tmpData);
    }
}