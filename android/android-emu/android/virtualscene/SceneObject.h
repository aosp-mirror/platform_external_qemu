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
 * Defines SceneObject, which represents an object in the 3D scene.
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
    SceneObject();

public:
    ~SceneObject();

    // Loads an object mesh from an .obj file.
    //
    // |renderer| - Renderer context, VirtualSceneObject will be bound to this
    //              context.
    // |filename| - Filename to load.
    //
    // Returns a SceneObject instance if the object could be loaded or null if
    // there was an error.
    static std::unique_ptr<SceneObject> loadFromObj(Renderer& renderer,
                                                    const char* filename);

    // Sets the model transform.
    //
    // |transform| - Model transform for this object.
    void setTransform(const glm::mat4& transform);

    // Gets the model transform for this object.
    glm::mat4 getTransform() const;

    const std::vector<Renderable>& getRenderables() const;

private:
    glm::mat4 mTransform = glm::mat4();
    std::vector<Renderable> mRenderables;
};

}  // namespace virtualscene
}  // namespace android
