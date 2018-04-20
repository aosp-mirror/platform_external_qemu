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

#include "android/base/files/PathUtils.h"
#include "android/base/system/System.h"
#include "android/camera/camera-metrics.h"
#include "android/utils/debug.h"
#include "android/virtualscene/MeshSceneObject.h"
#include "android/virtualscene/Renderer.h"

#include <cmath>

using namespace android::base;
using android::camera::CameraMetrics;

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

static constexpr const char* kObjFiles[] = {
        "Toren1BD.obj",
};

// static_cast the value in a unique_ptr.
// After this call, the unique_ptr that the value is cast from will be removed.
template <typename To, typename From>
std::unique_ptr<To> static_unique_cast(std::unique_ptr<From>& from) {
    return std::unique_ptr<To>(static_cast<To*>(from.release()));
}

namespace android {
namespace virtualscene {

Scene::Scene(Renderer& renderer) : mRenderer(renderer) {}

Scene::~Scene() = default;

std::unique_ptr<Scene> Scene::create(Renderer& renderer) {
    std::unique_ptr<Scene> scene;
    scene.reset(new Scene(renderer));
    if (!scene || !scene->initialize()) {
        return nullptr;
    }

    return scene;
}

bool Scene::initialize() {
    CameraMetrics::instance().setVirtualSceneName(kObjFiles[0]);

    mCamera.setAspectRatio(mRenderer.getAspectRatio());

    for (const char* objFile : kObjFiles) {
        std::unique_ptr<MeshSceneObject> sceneObject =
                MeshSceneObject::load(mRenderer, objFile);
        if (!sceneObject) {
            return false;
        }

        mSceneObjects.push_back(
                std::move(static_unique_cast<SceneObject>(sceneObject)));
    }

    return true;
}

void Scene::releaseResources() {
    mSceneObjects.clear();

    for (auto& poster : mPosters) {
        mRenderer.releaseTexture(poster.second.texture);
        mRenderer.releaseTexture(poster.second.defaultTexture);

        if (poster.second.sceneObject) {
            poster.second.sceneObject.reset();
        }
    }
}

const SceneCamera& Scene::getCamera() const {
    return mCamera;
}

int64_t Scene::update() {
    mCamera.update();
    for (auto& poster : mPosters) {
        poster.second.sceneObject->update();
    }

    return mCamera.getTimestamp();
}

std::vector<RenderableObject> Scene::getRenderableObjects() const {
    std::vector<RenderableObject> renderables;

    const glm::mat4 viewProjection = getCamera().getViewProjection();
    for (auto& sceneObject : mSceneObjects) {
        getRenderableObjectsFromSceneObject(viewProjection, sceneObject.get(),
                                            renderables);
    }

    for (auto& poster : mPosters) {
        if (poster.second.sceneObject) {
            getRenderableObjectsFromSceneObject(viewProjection,
                                                poster.second.sceneObject.get(),
                                                renderables);
        }
    }

    return std::move(renderables);
}

bool Scene::createPosterLocation(const PosterInfo& info) {
    PosterStorage storage;
    storage.sceneObject =
            PosterSceneObject::create(mRenderer, info.position, info.rotation,
                                      kPosterMinimumSizeMeters, info.size);
    if (!storage.sceneObject) {
        W("%s: Failed to create poster scene object %s.", __FUNCTION__,
          info.name.c_str());
        return false;
    }

    if (!info.defaultFilename.empty()) {
        storage.defaultTexture =
                mRenderer.loadTextureAsync(info.defaultFilename.c_str());
        storage.sceneObject->setTexture(storage.defaultTexture);
    }

    mPosters.insert(std::make_pair(info.name, std::move(storage)));
    return true;
}

bool Scene::loadPoster(const char* posterName,
                       const char* filename,
                       float scale,
                       LoadBehavior loadBehavior) {
    auto it = mPosters.find(posterName);
    if (it == mPosters.end()) {
        W("%s: Could not find poster with name '%s'", __FUNCTION__, posterName);
        return false;
    }

    PosterStorage& poster = it->second;

    mRenderer.releaseTexture(poster.texture);
    poster.texture = Texture();

    if (filename) {
        poster.texture = loadBehavior == LoadBehavior::Synchronous
                                 ? mRenderer.loadTexture(filename)
                                 : mRenderer.loadTextureAsync(filename);
    }

    poster.sceneObject->setScale(scale);
    poster.sceneObject->setTexture(
            poster.texture.isValid() ? poster.texture : poster.defaultTexture);

    return true;
}

void Scene::updatePosterScale(const char* posterName, float scale) {
    auto it = mPosters.find(posterName);
    if (it == mPosters.end()) {
        W("%s: Could not find poster with name '%s'", __FUNCTION__, posterName);
        return;
    }

    PosterStorage& poster = it->second;
    poster.sceneObject->setScale(scale);
}

void Scene::getRenderableObjectsFromSceneObject(
        const glm::mat4& viewProjection,
        const SceneObject* sceneObject,
        std::vector<RenderableObject>& outRenderableObjects) {
    if (sceneObject->isVisible()) {
        const glm::mat4 mvp = viewProjection * sceneObject->getTransform();

        for (const Renderable& renderable : sceneObject->getRenderables()) {
            outRenderableObjects.push_back({mvp, renderable});
        }
    }
}

}  // namespace virtualscene
}  // namespace android
