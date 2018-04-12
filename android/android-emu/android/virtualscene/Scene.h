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
#include "android/virtualscene/PosterSceneObject.h"
#include "android/virtualscene/Renderer.h"
#include "android/virtualscene/SceneCamera.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace android {
namespace virtualscene {

// Forward declarations.
class SceneObject;

struct Poster {
    glm::vec2 size = glm::vec2(1.0f, 1.0f);
    glm::vec3 position = glm::vec3();
    glm::quat rotation = glm::quat();
    std::string defaultFilename;

    std::unique_ptr<PosterSceneObject> sceneObject;
    Texture texture;
    Texture defaultTexture;
};

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

    // Before teardown, release all Renderer resources and SceneObjects.
    void releaseResources();

    // Get the scene camera.
    const SceneCamera& getCamera() const;

    // Update the scene for the next frame.
    //
    // Returns the timestamp for the frame.
    int64_t update();

    // Get the list of RenderableObjects for the current frame.
    std::vector<RenderableObject> getRenderableObjects() const;

    // Load a poster into the scene from a file.
    //
    // |posterName| - Name of the poster position, such as "wall" or "table".
    // |filename| - Path to an image file, either PNG or JPEG.
    // |size| - The default poster size, between 0 and 1, which will
    //          automatically be clamped.
    //
    // Returns true on success.
    bool loadPoster(const char* posterName, const char* filename, float size);

    // Update a given poster's size.  If the poster does not exist, this has
    // no effect.
    //
    // |posterName| - Name of the poster position, such as "wall" or "table".
    // |size| - Poster size, between 0 and 1, which will be automatically
    //          clamped.
    void updatePosterSize(const char* posterName, float size);

private:
    // Private constructor, use Scene::create to create an instance.
    Scene(Renderer& renderer);

    // Load the scene and create SceneObjects.
    //
    // Returns true on success.
    bool initialize();

    // Load a .posters file and populate the mPosters map.
    //
    // Returns true on success.
    bool loadPostersFile(const char* filename);

    // Gets RenderableObjects from a SceneObject.
    static void getRenderableObjectsFromSceneObject(
            const glm::mat4& viewProjection,
            const SceneObject* sceneObject,
            std::vector<RenderableObject>& outRenderableObjects);

    Renderer& mRenderer;
    SceneCamera mCamera;
    std::vector<std::unique_ptr<SceneObject>> mSceneObjects;
    std::unordered_map<std::string, Poster> mPosters;
};

}  // namespace virtualscene
}  // namespace android
