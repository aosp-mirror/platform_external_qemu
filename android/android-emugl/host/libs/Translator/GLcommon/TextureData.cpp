/*
* Copyright (C) 2011 The Android Open Source Project
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

#include "GLcommon/TextureData.h"

#include "android/base/files/StreamSerializing.h"
#include "GLcommon/GLEScontext.h"
#include "GLcommon/GLutils.h"

#include <cassert>

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

TextureData::TextureData(android::base::Stream* stream) : ObjectData(stream) {
    // The current TextureData structure is wrong when dealing with mipmaps.
    target = stream->getBe32();
    width = stream->getBe32();
    height = stream->getBe32();
    depth = stream->getBe32();
    border = stream->getBe32();
    internalFormat = stream->getBe32();
    format = stream->getBe32();
    type = stream->getBe32();
    if (target == GL_TEXTURE_2D) { // we only support GL_TEXTURE_2D for now
        loadBuffer(stream, &loadedData);
    }
    sourceEGLImage = stream->getBe32();
    hasStorage = stream->getByte();
    wasBound = stream->getByte();
    requiresAutoMipmap = stream->getByte();
    compressed = stream->getByte();
    compressedFormat = stream->getBe32();
    stream->read(crop_rect, sizeof(crop_rect));
    loadCollection(stream, &m_texParam,
            [](android::base::Stream* stream) {
                GLenum item = stream->getBe32();
                GLint val = stream->getBe32();
                return std::make_pair(item, val);
    });
}

void TextureData::onSave(android::base::Stream* stream) const {
    ObjectData::onSave(stream);
    // The current TextureData structure is wrong when dealing with mipmaps.
    stream->putBe32(target);
    stream->putBe32(width);
    stream->putBe32(height);
    stream->putBe32(depth);
    stream->putBe32(border);
    stream->putBe32(internalFormat);
    stream->putBe32(format);
    stream->putBe32(type);
    // consider getting data with glGetTexImage
    if (target == GL_TEXTURE_2D) {
        GLDispatch& dispatcher = GLEScontext::dispatcher();
        GLint prevPack = 4;
        GLint prevTex = 0;
        dispatcher.glGetIntegerv(GL_PACK_ALIGNMENT, &prevPack);
        dispatcher.glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
        dispatcher.glPixelStorei(GL_PACK_ALIGNMENT, 1);
        dispatcher.glBindTexture(GL_TEXTURE_2D, globalName);
        std::vector<unsigned char> tmpData(s_texImageSize(format, type, 1, width, height));
        dispatcher.glGetTexImage(target, 0, format, type, tmpData.data());
        dispatcher.glPixelStorei(GL_PACK_ALIGNMENT, prevPack);
        dispatcher.glBindTexture(GL_TEXTURE_2D, prevTex);
        saveBuffer(stream, tmpData);
    }
    stream->putBe32(sourceEGLImage);
    if (sourceEGLImage) {
        fprintf(stderr, "TextureData::onSave: warning: snapshotting EglImage,"
                " not supported\n");
    }
    stream->putByte(hasStorage);
    stream->putByte(wasBound);
    stream->putByte(requiresAutoMipmap);
    stream->putByte(compressed);
    stream->putBe32(compressedFormat);
    stream->write(crop_rect, sizeof(crop_rect));
    saveCollection(stream, m_texParam, [](android::base::Stream* stream,
                const std::pair<const GLenum, GLint>& texParam) {
                    stream->putBe32(texParam.first);
                    stream->putBe32(texParam.second);
    });
}

void TextureData::restore(ObjectLocalName localName,
            getGlobalName_t getGlobalName) {
    if (!target) return; // The texture never got bound
    if (target != GL_TEXTURE_2D) {
        fprintf(stderr,
                "warning: Non 2D texture target not supported for snapshot\n");
        return;
    }
    int globalName = getGlobalName(NamedObjectType::TEXTURE, localName);
    assert(globalName);
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    dispatcher.glBindTexture(target, globalName);
    // TODO: snapshot mipmaps
    GLint prevUnpack = 4;
    dispatcher.glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevUnpack);
    dispatcher.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    const void* pixels = loadedData.empty() ? nullptr : loadedData.data();
    dispatcher.glTexImage2D(target, 0, internalFormat, width,
            height, border, format, type, pixels);
    if (sourceEGLImage) {
        fprintf(stderr, "warning: this texture holds an Egl image handle,"
                " Egl image is not supported for snapshot\n");
    }
    loadedData.clear();
    for (const auto& texParam : m_texParam) {
        dispatcher.glTexParameteri(target, texParam.first, texParam.second);
    }
    dispatcher.glPixelStorei(GL_UNPACK_ALIGNMENT, prevUnpack);
}

void TextureData::setTexParam(GLenum pname, GLint param) {
    m_texParam[pname] = param;
}
