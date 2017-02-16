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

#define MAG_BEG 201
#define MAG_END 202

TextureData::TextureData(android::base::Stream* stream) : ObjectData(stream) {
    // The current TextureData structure is wrong when dealing with mipmaps.
    int mag = stream->getBe32();
    assert(mag == MAG_BEG);
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
    mag = stream->getBe32();
    assert(mag == MAG_END);
}

void TextureData::onSave(android::base::Stream* stream) const {
    ObjectData::onSave(stream);
    stream->putBe32(MAG_BEG);
    // The current TextureData structure is wrong when dealing with mipmaps.
    stream->putBe32(target);
    stream->putBe32(width);
    stream->putBe32(height);
    stream->putBe32(depth);
    stream->putBe32(border);
    // TODO: do we care about internal format?
    //stream->putBe32(internalFormat);
    stream->putBe32(format);
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
        std::vector<unsigned char> tmpData(texImageSize(format, type, 1, width, height));
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
    stream->putBe32(MAG_END);
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
