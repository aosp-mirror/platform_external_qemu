/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

/*
 * Defines Texture, which loads images from file and create OpenGL textures.
 */

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/base/Compiler.h"
#include "android/utils/compiler.h"

#include <memory>
#include <vector>

namespace android {
namespace virtualscene {

class Texture {
    DISALLOW_COPY_AND_ASSIGN(Texture);

public:
    ~Texture();

    // Load a PNG image from a file and create an OpenGL texture from it.
    //
    // The lifetime of the OpenGL texture is determined by the lifetime of the
    // Texture class; the texture id will be released when the Texture is
    // destroyed.
    //
    // |gles2| - Pointer to GLESv2Dispatch, must be non-null.
    // |filename| - Filename to load.
    //
    // Returns a Texture instance if the texture could be loaded or null if
    // there was an error.
    static std::unique_ptr<Texture> load(const GLESv2Dispatch* gles2,
                                         const char* filename);

    // Create an OpenGL texture with the given width and height with storage
    // allocated, but no data loaded.
    //
    // The lifetime of the OpenGL texture is determined by the lifetime of the
    // Texture class; the texture id will be released when the Texture is
    // destroyed.
    //
    // |gles2| - Pointer to GLESv2Dispatch, must be non-null.
    // |width| - Requested texture width.
    // |height| - Requested texture height.
    //
    // Returns a Texture instance if the texture could be created or null if
    // there was an error.
    static std::unique_ptr<Texture> createEmpty(const GLESv2Dispatch* gles2,
                                                uint32_t width,
                                                uint32_t height);

    // Returns the texture id of the loaded texture.
    GLuint getTextureId() const;

private:
    // Private constructor, use Texture::load to create an instance.
    Texture(const GLESv2Dispatch* gles2, GLuint textureId);

    // Checks whether the given height and width are appropriate for a texture.
    static bool isTextureSizeValid(const GLESv2Dispatch* gles2,
                                   uint32_t width,
                                   uint32_t height);

    static bool loadPNG(const char* filename,
                        std::vector<uint8_t>& buffer,
                        uint32_t* outWidth,
                        uint32_t* outHeight);

    const GLESv2Dispatch* mGles2 = nullptr;
    GLuint mTextureId = 0;
};

}  // namespace virtualscene
}  // namespace android
