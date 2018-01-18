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
 * The Scene container for the Virtual Scene.
 */

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/utils/compiler.h"
#include "android/virtualscene/Renderer.h"
#include "android/virtualscene/SceneCamera.h"

#include <memory>
#include <vector>

namespace android {
namespace virtualscene {

// Forward declarations.
class SceneObject;

class Scene {
    DISALLOW_COPY_AND_ASSIGN(Scene);

public:
    ~Scene();

    // Create a Scene.
    //
    // |gles2| - Pointer to GLESv2Dispatch, must be non-null.
    //
    // Returns a Scene instance if the scene was successfully created or
    // null if there was an error.
    static std::unique_ptr<Scene> create(Renderer& renderer);

    // Before teardown, release all SceneObjects so that Renderer resources are
    // cleaned up.
    void releaseSceneObjects(Renderer& renderer);

    // Get the scene camera.
    const SceneCamera& getCamera() const;

    // Update the scene for the next frame.
    //
    // Returns the timestamp for the frame.
    int64_t update();

    // Get the list of RenderableObjects for the current frame.
    std::vector<RenderableObject> getRenderableObjects() const;

private:
    // Private constructor, use Scene::create to create an instance.
    Scene();

    // Load the scene and create SceneObjects.
    bool initialize(Renderer& renderer);

    SceneCamera mCamera;
    std::vector<std::unique_ptr<SceneObject>> mSceneObjects;
};

}  // namespace virtualscene
}  // namespace android
