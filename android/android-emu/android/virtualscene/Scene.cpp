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

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <cmath>
#include <fstream>
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

static constexpr const char* kPosterFile = "Toren1BD.posters";

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

    if (!loadPostersFile(kPosterFile)) {
        return false;
    }

    for (auto& poster : mPosters) {
        poster.second.sceneObject = PosterSceneObject::create(
                mRenderer, poster.second.position, poster.second.rotation,
                poster.second.size);
        if (!poster.second.sceneObject) {
            W("%s: Failed to create poster scene object %s.", __FUNCTION__,
              poster.first.c_str());
            return false;
        }

        if (!poster.second.defaultFilename.empty()) {
            poster.second.defaultTexture = mRenderer.loadTextureAsync(
                    poster.second.defaultFilename.c_str());
            poster.second.sceneObject->setTexture(poster.second.defaultTexture);
        }
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
        if (poster.second.sceneObject) {
            poster.second.sceneObject->update();
        }
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

bool Scene::loadPoster(const char* posterName,
                       const char* filename,
                       float size) {
    auto it = mPosters.find(posterName);
    if (it == mPosters.end()) {
        W("%s: Could not find poster with name '%s'", __FUNCTION__, posterName);
        return false;
    }

    Poster& poster = it->second;

    mRenderer.releaseTexture(poster.texture);
    poster.texture = Texture();

    if (filename) {
        poster.texture = mRenderer.loadTextureAsync(filename);
    }

    poster.sceneObject->setScale(size);
    poster.sceneObject->setTexture(
            poster.texture.isValid() ? poster.texture : poster.defaultTexture);

    return true;
}

void Scene::updatePosterSize(const char* posterName, float size) {
    auto it = mPosters.find(posterName);
    if (it == mPosters.end()) {
        W("%s: Could not find poster with name '%s'", __FUNCTION__, posterName);
        return;
    }

    Poster& poster = it->second;
    poster.sceneObject->setScale(size);
}

bool Scene::loadPostersFile(const char* filename) {
    const std::string resourcesDir =
            PathUtils::addTrailingDirSeparator(PathUtils::join(
                    System::get()->getLauncherDirectory(), "resources"));
    const std::string filePath = PathUtils::join(
            System::get()->getLauncherDirectory(), "resources", filename);

    std::ifstream in(filePath);
    if (!in) {
        W("%s: Could not find file '%s'", __FUNCTION__, filename);
        return false;
    }

    Poster poster;
    std::string name;

    std::string str;

    for (in >> str; !in.eof(); in >> str) {
        if (str.empty()) {
            continue;
        }

        if (str == "poster") {
            // New poster entry, specified with a string name.
            if (!name.empty()) {
                // Store existing poster.
                D("%s: Loaded poster %s at (%f, %f, %f)", __FUNCTION__,
                  name.c_str(), poster.position.x, poster.position.y,
                  poster.position.z);
                mPosters[name] = std::move(poster);
            }

            poster = Poster();
            in >> name;
            if (!in) {
                W("%s: Invalid name.", __FUNCTION__);
                return false;
            }

        } else if (str == "position") {
            // Poster center position.
            // Specified with three floating point numbers, separated by
            // whitespace.

            in >> poster.position.x >> poster.position.y >> poster.position.z;
            if (!in) {
                W("%s: Invalid position.", __FUNCTION__);
                return false;
            }
        } else if (str == "rotation") {
            // Poster rotation.
            // Specified with three floating point numbers, separated by
            // whitespace.  This represents euler angle rotation in degrees, and
            // it is applied in XYZ order.

            glm::vec3 eulerRotation;
            in >> eulerRotation.x >> eulerRotation.y >> eulerRotation.z;
            if (!in) {
                W("%s: Invalid rotation.", __FUNCTION__);
                return false;
            }

            poster.rotation = glm::quat(
                    glm::eulerAngleXYZ(glm::radians(eulerRotation.x),
                                       glm::radians(eulerRotation.y),
                                       glm::radians(eulerRotation.z)));
        } else if (str == "size") {
            // Poster center position.
            // Specified with two floating point numbers, separated by
            // whitespace.

            in >> poster.size.x >> poster.size.y;
            if (!in) {
                W("%s: Invalid size.", __FUNCTION__);
                return false;
            }
        } else if (str == "default") {
            // Poster default filename.
            // Specified with a string parameter.

            in >> poster.defaultFilename;
            if (!in) {
                W("%s: Invalid default filename.", __FUNCTION__);
                return false;
            }
        } else {
            W("%s: Invalid input %s", __FUNCTION__, str.c_str());
            return false;
        }
    }

    if (name.empty()) {
        E("%s: Posters file did not contain any entries.", __FUNCTION__);
        return false;
    }

    // Store last poster.
    D("%s: Loaded poster %s at (%f, %f, %f)", __FUNCTION__, name.c_str(),
      poster.position.x, poster.position.y, poster.position.z);
    mPosters[name] = std::move(poster);

    return true;
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
