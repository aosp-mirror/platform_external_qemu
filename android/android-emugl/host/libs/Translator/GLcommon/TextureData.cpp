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

TextureData::TextureData(android::base::Stream* stream) : ObjectData(stream) {
    // The current TextureData structure is wrong when dealing with mipmaps.
    width = stream->getBe32();
    height = stream->getBe32();
    depth = stream->getBe32();
    border = stream->getBe32();
    internalFormat = stream->getBe32();
    format = stream->getBe32();
    type = stream->getBe32();
    loadBuffer(stream, &data);
    sourceEGLImage = stream->getBe32();
    hasStorage = stream->getByte();
    wasBound = stream->getByte();
    requiresAutoMipmap = stream->getByte();
    compressed = stream->getByte();
    compressedFormat = stream->getBe32();
    stream->read(crop_rect, sizeof(crop_rect));
    target = stream->getBe32();
}

void TextureData::onSave(android::base::Stream* stream) const {
    ObjectData::onSave(stream);
    // The current TextureData structure is wrong when dealing with mipmaps.
    stream->putBe32(width);
    stream->putBe32(height);
    stream->putBe32(depth);
    stream->putBe32(border);
    stream->putBe32(internalFormat);
    stream->putBe32(format);
    stream->putBe32(type);
    // consider getting data with glGetTexImage
    saveBuffer(stream, data);
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
    stream->putBe32(target);
}

void TextureData::restore(ObjectLocalName localName,
            getGlobalName_t getGlobalName) {
    GLenum err = 0;
    int globalName = getGlobalName(NamedObjectType::TEXTURE, localName);
    GLDispatch& dispatcher = GLEScontext::dispatcher();
    _DEBUG_ERR
    dispatcher.glBindTexture(target, globalName);
    _DEBUG_ERR
    // TODO: snapshot mipmaps
    // TODO: snapshot glTexParameter stuff
    // TODO: handle glCopyTexImage2D
    dispatcher.glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    _DEBUG_ERR
    const void* pixels = data.empty() ? nullptr : data.data();
    dispatcher.glTexImage2D(target, 0, internalFormat, width,
            height, border, format, type, pixels);
    fprintf(stderr, "format %d, type %d\n", format, type);
    if (sourceEGLImage) {
        fprintf(stderr, "warning: this texture holds an egl image handle\n");
    }
    _DEBUG_ERR
}
