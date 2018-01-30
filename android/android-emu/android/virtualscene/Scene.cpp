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

#include "android/virtualscene/Scene.h"

#include "android/camera/camera-metrics.h"
#include "android/utils/debug.h"
#include "android/virtualscene/Renderer.h"
#include "android/virtualscene/SceneObject.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <vector>

using namespace android::base;
using android::camera::CameraMetrics;

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

static constexpr const char* kObjFiles[] = {
        "Toren1BD.obj",
};

namespace android {
namespace virtualscene {

Scene::Scene() = default;
Scene::~Scene() = default;

std::unique_ptr<Scene> Scene::create(Renderer& renderer) {
    std::unique_ptr<Scene> scene;
    scene.reset(new Scene());
    if (!scene || !scene->initialize(renderer)) {
        return nullptr;
    }

    return scene;
}

bool Scene::initialize(Renderer& renderer) {
    CameraMetrics::instance().setVirtualSceneName(kObjFiles[0]);

    mCamera.setAspectRatio(renderer.getAspectRatio());

    for (const char* objFile : kObjFiles) {
        std::unique_ptr<SceneObject> sceneObject =
                SceneObject::loadFromObj(renderer, objFile);
        if (!sceneObject) {
            return false;
        }

        mSceneObjects.push_back(std::move(sceneObject));
    }

    return true;
}

void Scene::releaseSceneObjects(Renderer& renderer) {
    for (auto& sceneObject : mSceneObjects) {
        renderer.releaseObjectResources(sceneObject.get());
    }

    mSceneObjects.clear();
}

const SceneCamera& Scene::getCamera() const {
    return mCamera;
}

int64_t Scene::update() {
    mCamera.update();
    return mCamera.getTimestamp();
}

std::vector<RenderableObject> Scene::getRenderableObjects() const {
    std::vector<RenderableObject> renderables;

    for (auto& sceneObject : mSceneObjects) {
        const glm::mat4 mvp =
                getCamera().getViewProjection() * sceneObject->getTransform();

        for (const Renderable& renderable : sceneObject->getRenderables()) {
            renderables.push_back({mvp, renderable});
        }
    }

    return std::move(renderables);
}

}  // namespace virtualscene
}  // namespace android
