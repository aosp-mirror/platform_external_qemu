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

#pragma once

#include "android/base/files/Stream.h"

#include "GLcommon/ObjectData.h"

#include <memory>
#include <unordered_map>
#include <vector>

class SaveableTexture;
using SaveableTexturePtr = std::shared_ptr<SaveableTexture>;

class TextureData : public ObjectData {
public:
    TextureData() : ObjectData(TEXTURE_DATA) { resetSaveableTexture(); };
    TextureData(android::base::Stream* stream);

    unsigned int width{0};
    unsigned int height{0};
    unsigned int depth{0};
    unsigned int border{0};
    unsigned int internalFormat{GL_RGBA};
    // TODO: store emulated internal format
    unsigned int format{GL_RGBA};
    unsigned int type{GL_UNSIGNED_BYTE};
    unsigned int sourceEGLImage{0};
    bool hasStorage{false};
    bool wasBound{false};
    bool requiresAutoMipmap{false};
    bool compressed{false};
    unsigned int compressedFormat{0};
    int32_t crop_rect[4] = {};
    GLenum target{0};
    // texStorageLevels tracks the storage level explicitly set by
    // glTexStorage* (GLES3)
    // maxMipmapLevel tracks the implicit maximum mipmap level that needs to
    // be snapshot (GLES2 and GLES3)
    // They are very similar concepts. But texStorageLevels is GLES3 only (only
    // for textures initialized with glTexStorage*), and maxMipmapLevel is for
    // both GLES2 and 3.
    unsigned int texStorageLevels = 0;
    int samples;
    // globalName is used for snapshot when reading data from GPU
    int globalName = 0;
    void onSave(android::base::Stream* stream,
                unsigned int globalName) const override;
    void restore(ObjectLocalName localName,
                 const getGlobalName_t& getGlobalName) override;
    void setSaveableTexture(SaveableTexturePtr&& saveableTexture);
    const SaveableTexturePtr& getSaveableTexture() const;
    void resetSaveableTexture();
    void setTexParam(GLenum pname, GLint param);
    GLenum getSwizzle(GLenum component) const;
    void makeDirty();
    void setTarget(GLenum _target);
    void setMipmapLevelAtLeast(unsigned int level);

protected:
    std::unordered_map<GLenum, GLint> m_texParam;
    SaveableTexturePtr m_saveableTexture;
};
