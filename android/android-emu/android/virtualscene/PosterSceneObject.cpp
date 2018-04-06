/*
 * Copyright (C) 2018 The Android Open Source Project
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

#include "android/virtualscene/PosterSceneObject.h"

#include "android/utils/debug.h"
#include "android/virtualscene/Renderer.h"

#include <glm/gtc/matrix_transform.hpp>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

static constexpr android::virtualscene::VertexPositionUV kQuadVerts[] = {
        {glm::vec3(-0.5f, -0.5f, 0.f), glm::vec2(0.f, 0.f)},
        {glm::vec3(0.5f, -0.5f, 0.f), glm::vec2(1.f, 0.f)},
        {glm::vec3(-0.5f, 0.5f, 0.f), glm::vec2(0.f, 1.f)},
        {glm::vec3(0.5f, 0.5f, 0.f), glm::vec2(1.f, 1.f)},
};

static constexpr GLuint kQuadIndices[] = {0, 1, 2, 2, 1, 3};

static constexpr float kMinimumSizeMeters = 0.2f;

namespace android {
namespace virtualscene {

PosterSceneObject::PosterSceneObject() = default;

PosterSceneObject::~PosterSceneObject() = default;

std::unique_ptr<PosterSceneObject> PosterSceneObject::create(
        Renderer& renderer,
        const char* textureFilename,
        const glm::vec3& position,
        const glm::quat& rotation,
        const glm::vec2& maxSize) {
    // Create an empty PosterSceneObject so that the texture can be attached
    // to something.
    std::unique_ptr<PosterSceneObject> result(new PosterSceneObject());

    Texture texture = renderer.loadTexture(result.get(), textureFilename);
    if (!texture.isValid()) {
        W("%s: Could not load texture '%s'", __FUNCTION__, textureFilename);
        return nullptr;
    }

    // Determine poster size.
    uint32_t width = 0;
    uint32_t height = 0;
    renderer.getTextureInfo(texture, &width, &height);

    glm::vec2 aspectRatio =
            glm::vec2(static_cast<float>(width), static_cast<float>(height));
    const float aspectRatioMax = std::max(aspectRatio.x, aspectRatio.y);
    if (aspectRatioMax > 0.0f) {
        aspectRatio /= aspectRatioMax;
    }

    const glm::vec3 scale(maxSize * aspectRatio, 1.0f);
    const glm::mat4 baseTransform = glm::translate(glm::mat4(), position) *
                                    glm::mat4_cast(rotation) *
                                    glm::scale(glm::mat4(), scale);

    // The minimum scale is the minimum size that the poster can be without
    // being smaller than kMinimumSizeMeters (on the smallest side).
    const float posterSize = std::min(scale.x, scale.y);
    float minScale = kMinimumSizeMeters / posterSize;
    if (minScale > 1.0f) {
        // This indicates that a poster in the scene is smaller than the minimum
        // size.  Set to 1 to effectively disable scaling and avoid exceeding
        // the poster's bounds.
        minScale = 1.0f;
    }

    // Initialize result.
    Renderable renderable;
    renderable.material = renderer.createMaterialTextured();
    renderable.mesh =
            renderer.createMesh(result.get(), kQuadVerts, kQuadIndices);
    renderable.texture = texture;

    result.get()->mRenderables.emplace_back(std::move(renderable));
    result.get()->mBaseTransform = baseTransform;
    result.get()->mMinScale = minScale;
    result.get()->updateTransform();

    return result;
}

void PosterSceneObject::setScale(float value) {
    mScale = value;
    updateTransform();
}

void PosterSceneObject::updateTransform() {
    const float scale = glm::clamp(mScale, mMinScale, 1.0f);
    setTransform(mBaseTransform * glm::scale(glm::mat4(), glm::vec3(scale)));
}

}  // namespace virtualscene
}  // namespace android
