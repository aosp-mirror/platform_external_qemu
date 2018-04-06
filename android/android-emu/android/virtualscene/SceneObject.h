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
 * Defines SceneObject, which represents an object in the 3D scene.  To render
 * content, use a class that derives SceneObject and adds content, such as
 * MeshSceneObject.
 */

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/base/Compiler.h"
#include "android/utils/compiler.h"
#include "android/virtualscene/Renderer.h"

#include <glm/glm.hpp>

namespace android {
namespace virtualscene {

class SceneObject {
    DISALLOW_COPY_AND_ASSIGN(SceneObject);

protected:
    SceneObject(Renderer& renderer);

public:
    virtual ~SceneObject();

    // Sets the model transform.
    //
    // |transform| - Model transform for this object.
    void setTransform(const glm::mat4& transform);

    // Gets the model transform for this object.
    glm::mat4 getTransform() const;

    // Get Renderables for the scene object.
    const std::vector<Renderable>& getRenderables() const;

    // Returns true if the SceneObject is visible.
    bool isVisible() const;

protected:
    Renderer& mRenderer;

    glm::mat4 mTransform = glm::mat4();
    std::vector<Renderable> mRenderables;
    bool mVisible = true;
};

}  // namespace virtualscene
}  // namespace android
