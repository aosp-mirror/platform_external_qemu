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
                    target(0),
                    m_isDirty(true) {};
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
    unsigned int texStorageLevels = 0;
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
    bool isDirty() const;
    void makeDirty();
    void makeClean();
protected:
    std::unordered_map<GLenum, GLint> m_texParam;
    SaveableTexturePtr m_saveableTexture;
    bool m_isDirty = true;
};
