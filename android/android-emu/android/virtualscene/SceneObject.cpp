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

#include "android/virtualscene/SceneObject.h"

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/utils/debug.h"

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

namespace android {
namespace virtualscene {

SceneObject::SceneObject(Renderer& renderer) : mRenderer(renderer) {}

SceneObject::~SceneObject() {
    for (Renderable& renderable : mRenderables) {
        mRenderer.releaseMaterial(renderable.material);
        mRenderer.releaseMesh(renderable.mesh);
        mRenderer.releaseTexture(renderable.texture);
    }
}

void SceneObject::setTransform(const glm::mat4& transform) {
    mTransform = transform;
}

glm::mat4 SceneObject::getTransform() const {
    return mTransform;
}

const std::vector<Renderable>& SceneObject::getRenderables() const {
    return mRenderables;
}

bool SceneObject::isVisible() const {
    return mVisible;
}

}  // namespace virtualscene
}  // namespace android
