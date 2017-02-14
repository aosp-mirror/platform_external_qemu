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

TextureData::TextureData(android::base::Stream* stream) : ObjectData(stream) {
    // The current TextureData structure is wrong when dealing with mipmaps.
    width = stream->getBe32();
    height = stream->getBe32();
    depth = stream->getBe32();
    border = stream->getBe32();
    internalFormat = stream->getBe32();
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