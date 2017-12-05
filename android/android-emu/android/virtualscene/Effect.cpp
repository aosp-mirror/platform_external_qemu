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

#include "android/virtualscene/Effect.h"

#include "android/utils/debug.h"

#define W(...) dwarning(__VA_ARGS__)

namespace android {
namespace virtualscene {

static constexpr VertexPositionUV kScreenQuadVerts[] = {
    { glm::vec3(-1.f, -1.f, 0.f), glm::vec2(0.f,  0.f) },
    { glm::vec3( 3.f, -1.f, 0.f), glm::vec2(2.f,  0.f) },
    { glm::vec3(-1.f,  3.f, 0.f), glm::vec2(0.f,  2.f) },
};

static constexpr GLuint kScreenQuadIndices[] = {
    0, 1, 2,
};

Effect::Effect(const std::shared_ptr<Material>& material,
               const std::shared_ptr<Mesh>& mesh)
    : mMaterial(material), mMesh(mesh) {}
Effect::~Effect() = default;

std::unique_ptr<Effect> Effect::createEffect(Renderer& renderer,
                                             const char* frag) {
    std::shared_ptr<Material> material =
            renderer.createMaterialScreenSpace(frag);
    std::shared_ptr<Mesh> mesh =
            renderer.createMesh(kScreenQuadVerts, kScreenQuadIndices);

    if (!material || !mesh) {
        W("%s: Failed creating material or mesh.", __FUNCTION__);
        return nullptr;
    }

    std::unique_ptr<Effect> result(new Effect(material, mesh));
    return result;
}

RenderCommand Effect::getRenderCommand(
        const std::shared_ptr<Texture>& inputTexture) {
    return {mMaterial, inputTexture, mMesh};
}

}  // namespace virtualscene
}  // namespace android
