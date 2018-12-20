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

#include "android/base/containers/SmallVector.h"
#include "android/base/files/Stream.h"
#include "android/snapshot/LazySnapshotObj.h"
#include "GLcommon/NamedObject.h"
#include "GLcommon/TextureData.h"
#include "GLcommon/TranslatorIfaces.h"

#include <GLES2/gl2ext.h>

#include <atomic>
#include <functional>
#include <memory>

class GLDispatch;
class GlobalNameSpace;
class TextureData;

// TextureGlobal is an auxiliary class when save and load a texture / EglImage.
// We need it only for texture because texture is the only GL object that can be
// shared globally (i.e. across guest processes) by wrapping it up in EglImage.

// Lifespan:
// When saving a snapshot, TextureGlobal should be populated in the pre-save
// stage, save data in save stage and get destroyed right after that.
// When loading from a snapshot, TextureGlobal will be populated and restoring
// hardware texture before restoring any EglImage or GLES texture handles.
// EglImages and GLES textures get their global handles from TextureGlobal when
// EglImages and GLES textures are being loaded. Then TextureGlobal will be
// destroyed.

class SaveableTexture :
        public android::snapshot::LazySnapshotObj<SaveableTexture> {
public:
    using Buffer = android::base::SmallVector<unsigned char>;
    using saver_t = void (*)(SaveableTexture*,
                             android::base::Stream*,
                             Buffer* buffer);
    // loader_t is supposed to setup a stream and trigger loadFromStream.
    typedef std::function<void(SaveableTexture*)> loader_t;
    using creator_t = SaveableTexture* (*)(GlobalNameSpace*, loader_t&&);
    using restorer_t = void (*)(SaveableTexture*);

    SaveableTexture() = delete;
    SaveableTexture(SaveableTexture&&) = delete;
    SaveableTexture(GlobalNameSpace* globalNameSpace, loader_t&& loader);
    SaveableTexture& operator=(SaveableTexture&&) = delete;

    SaveableTexture(const TextureData& texture);
    // preSave and postSave should be called exactly once before and after
    // all texture saves.
    // The bound context cannot be changed from preSave to onSave to postSave
    static void preSave();
    static void postSave();
    // precondition: a context must be properly bound
    void onSave(android::base::Stream* stream);
    // getGlobalObject() will touch and load data onto GPU if it is not yet
    // restored
    const NamedObjectPtr& getGlobalObject();
    // precondition: a context must be properly bound
    void fillEglImage(EglImage* eglImage);
    void loadFromStream(android::base::Stream* stream);
    void makeDirty();
    bool isDirty() const;
    void setTarget(GLenum target);
    void setMipmapLevelAtLeast(unsigned int level);

    unsigned int getGlobalName();

    // precondition: (1) a context must be properly bound
    //               (2) m_fileReader is set up
    void restore();

private:
    unsigned int m_target = GL_TEXTURE_2D;
    unsigned int m_width = 0;
    unsigned int m_height = 0;
    unsigned int m_depth = 0;  // For texture 3D
    unsigned int m_format = GL_RGBA;
    unsigned int m_internalFormat = GL_RGBA;
    unsigned int m_type = GL_UNSIGNED_BYTE;
    unsigned int m_border = 0;
    unsigned int m_texStorageLevels = 0;
    unsigned int m_maxMipmapLevel = 0;
    // Attributes used when saving a snapshot
    unsigned int m_globalName = 0;
    // Attributes used when loaded from a snapshot
    NamedObjectPtr m_globalTexObj = nullptr;
    struct LevelImageData {
        unsigned int m_width = 0;
        unsigned int m_height = 0;
        unsigned int m_depth = 0;
        android::base::SmallFixedVector<unsigned char, 16> m_data;
    };
    std::unique_ptr<LevelImageData[]> m_levelData[6] = {};
    std::unordered_map<GLenum, GLint> m_texParam;
    loader_t m_loader;
    GlobalNameSpace* m_globalNamespace = nullptr;
    bool m_isDirty = true;
    std::atomic<bool> m_loadedFromStream { false };
};

typedef std::shared_ptr<SaveableTexture> SaveableTexturePtr;
