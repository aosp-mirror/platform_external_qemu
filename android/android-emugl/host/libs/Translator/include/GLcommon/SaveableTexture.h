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

#include "GLcommon/NamedObject.h"
#include "GLcommon/TextureData.h"
#include "GLcommon/TranslatorIfaces.h"
#include "android/base/files/Stream.h"

#include <memory>

class GLDispatch;
class GlobalNameSpace;

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

class SaveableTexture {
public:
    using Buffer = android::base::SmallVector<unsigned char>;
    using saver_t = void (*)(SaveableTexture*,
                             android::base::Stream*,
                             Buffer* buffer);
    using loader_t = SaveableTexture* (*)(android::base::Stream*,
                                          GlobalNameSpace*,
                                          Buffer* buffer);

    SaveableTexture() = delete;
    SaveableTexture(SaveableTexture&&) = delete;
    SaveableTexture& operator=(SaveableTexture&&) = delete;

    SaveableTexture(const EglImage& eglImage);
    SaveableTexture(const TextureData& texture);

    // precondition: a context must be properly bound
    SaveableTexture(android::base::Stream* stream,
                    GlobalNameSpace* globalNameSpace,
                    Buffer* buffer);
    // precondition: a context must be properly bound
    void onSave(android::base::Stream* stream,
                android::base::SmallVector<unsigned char>* buffer) const;
    const NamedObjectPtr& getGlobalObject() const { return m_globalTexObj; }
    EglImage* makeEglImage() const;
    // TODO: makeTextureData as well
private:
    unsigned int m_target = GL_TEXTURE_2D;
    unsigned int m_width = 0;
    unsigned int m_height = 0;
    unsigned int m_depth = 0;  // For texture 3D
    unsigned int m_format = GL_RGBA;
    unsigned int m_internalFormat = GL_RGBA;
    unsigned int m_type = GL_UNSIGNED_BYTE;
    unsigned int m_border = 0;
    // Attributes used when saving a snapshot
    unsigned int m_globalName = 0;
    // Attributes used when loaded from a snapshot
    NamedObjectPtr m_globalTexObj = nullptr;
};

typedef std::unique_ptr<SaveableTexture> SaveableTexturePtr;
