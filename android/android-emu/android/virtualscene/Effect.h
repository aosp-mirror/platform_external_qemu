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
 * Defines Effect, which represents an screen-space render effect.
 */

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/base/Compiler.h"
#include "android/utils/compiler.h"
#include "android/virtualscene/Renderer.h"

namespace android {
namespace virtualscene {

// Forward declarations
class Texture;

class Effect {
    DISALLOW_COPY_AND_ASSIGN(Effect);

public:
    ~Effect();

    // Create a screen space affect that draws a full screen quad.
    //
    // |gles2| - Pointer to GLESv2Dispatch, must be non-null.
    // |frag| - Frament shader to use for effect.  Shader should have a varying
    //          vec2 named 'resolution' which will be set to the inverse of the
    //          resolution to which the effect is rendered (i.e. (1/x, 1/y), and
    //          a sampler2D named tex_sampler which will be set to the input
    //          texture.
    //
    // Returns a RenderTarget instance if the successful or null if
    // there was an error.
    //
    static std::unique_ptr<Effect> createEffect(Renderer& renderer,
                                                const char* frag);

    // Render the effect on the input texture to a width * height view.
    RenderCommand getRenderCommand(
            const std::shared_ptr<Texture>& inputTexture);

private:
    // Private constructor, use Effect::createEffect to create an
    // instance.
    Effect(const std::shared_ptr<Material>& material,
           const std::shared_ptr<Mesh>& mesh);

    std::shared_ptr<Material> mMaterial;
    std::shared_ptr<Mesh> mMesh;
};

}  // namespace virtualscene
}  // namespace android
