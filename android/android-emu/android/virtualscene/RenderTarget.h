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
 * Defines a render target with all associated gl objects.
 */

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/base/Compiler.h"
#include "android/utils/compiler.h"

#include <memory>

namespace android {
namespace virtualscene {

// Forward declarations
class Texture;

class RenderTarget {
    DISALLOW_COPY_AND_ASSIGN(RenderTarget);

public:
    ~RenderTarget();

    // Create a Texture backed render target with the given size.
    //
    // |gles2| - Pointer to GLESv2Dispatch, must be non-null.
    // |width| - Requested texture width.
    // |height| - Requested texture height.
    //
    // Returns a RenderTarget instance if the successful or null if
    // there was an error.
    static std::unique_ptr<RenderTarget> createTextureTarget(
            const GLESv2Dispatch* gles2, uint32_t width, uint32_t height);

    // Create a render target referring to no framebuffer (i.e. use to render to
    // the current display).
    //
    // |gles2| - Pointer to GLESv2Dispatch, must be non-null.
    // |width| - Requested render width.
    // |height| - Requested render height.
    //
    // Returns a RenderTarget instance if the successful or null if
    // there was an error.
    static std::unique_ptr<RenderTarget> createDefault(
            const GLESv2Dispatch* gles2, uint32_t width, uint32_t height);

    // Binds the framebuffer of this RenderTarget and sets the viewport size to
    // the size set when this RenderTarget was created.
    void bind() const;

    // Returns the  texture backing this render target, or nullptr if this is
    // not a texture-backed target.
    const Texture* getTexture() const;

private:
    // Private constructor, use Texture::load to create an instance.
    RenderTarget(const GLESv2Dispatch* gles2,
                 GLuint framebuffer,
                 GLuint depthRenderbuffer,
                 uint32_t width,
                 uint32_t height);

    void setTexture(std::unique_ptr<Texture>&& texture);

    const GLESv2Dispatch* mGles2 = nullptr;

    GLuint mFramebuffer;
    GLuint mDepthRenderbuffer;
    std::unique_ptr<Texture> mTexture;

    uint32_t mWidth;
    uint32_t mHeight;
};

}  // namespace virtualscene
}  // namespace android
