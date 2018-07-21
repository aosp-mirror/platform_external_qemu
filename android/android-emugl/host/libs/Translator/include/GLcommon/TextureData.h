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
typedef std::shared_ptr<SaveableTexture> SaveableTexturePtr;

class TextureData : public ObjectData
{
public:
    TextureData():  ObjectData(TEXTURE_DATA),
                    width(0),
                    height(0),
                    depth(0),
                    border(0),
                    internalFormat(GL_RGBA),
                    format(GL_RGBA),
                    type(GL_UNSIGNED_BYTE),
                    sourceEGLImage(0),
                    hasStorage(false),
                    wasBound(false),
                    requiresAutoMipmap(false),
                    compressed(false),
                    compressedFormat(0),
                    target(0) { resetSaveableTexture(); };
    TextureData(android::base::Stream* stream);

    unsigned int width;
    unsigned int height;
    unsigned int depth;
    unsigned int border;
    unsigned int internalFormat;
    // TODO: store emulated internal format
    unsigned int format;
    unsigned int type;
    unsigned int sourceEGLImage;
    bool hasStorage;
    bool wasBound;
    bool requiresAutoMipmap;
    bool compressed;
    unsigned int compressedFormat;
    int32_t crop_rect[4] = {};
    GLenum target;
    // texStorageLevels tracks the storage level explicitly set by
    // glTexStorage* (GLES3)
    // maxMipmapLevel tracks the implicit maximum mipmap level that needs to
    // be snapshot (GLES2 and GLES3)
    // They are very similar concepts. But texStorageLevels is GLES3 only (only
    // for textures initialized with glTexStorage*), and maxMipmapLevel is for
    // both GLES2 and 3.
    unsigned int texStorageLevels = 0;
    int samples;
    void onSave(android::base::Stream* stream,
                unsigned int globalName) const override;
    void restore(ObjectLocalName localName,
                 const getGlobalName_t& getGlobalName) override;
    void setSaveableTexture(SaveableTexturePtr&& saveableTexture);
    const SaveableTexturePtr& getSaveableTexture() const;
    void resetSaveableTexture();

    unsigned int getGlobalName() const;
    void setGlobalName(unsigned name);

    // deprecated; texture parameters are dealt with by SaveableTexture
    void setTexParam(GLenum pname, GLint param);
    GLenum getSwizzle(GLenum component) const;

    void makeDirty();
    void setTarget(GLenum _target);
    void setMipmapLevelAtLeast(unsigned int level);
protected:
    // globalName is set during bind, used for snapshot when reading data from
    // GPU Usually this should be kept by saveableTexture, but we might not have
    // one yet.
    unsigned int globalName = 0;

    // deprecated; texture parameters are dealt with by SaveableTexture
    std::unordered_map<GLenum, GLint> m_texParam;

    SaveableTexturePtr m_saveableTexture;
};
