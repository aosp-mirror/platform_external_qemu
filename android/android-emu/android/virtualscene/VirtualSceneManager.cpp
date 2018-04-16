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

#include "android/virtualscene/VirtualSceneManager.h"

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/base/files/PathUtils.h"
#include "android/cmdline-option.h"
#include "android/featurecontrol/FeatureControl.h"
#include "android/globals.h"
#include "android/skin/winsys.h"
#include "android/utils/debug.h"
#include "android/virtualscene/Renderer.h"
#include "android/virtualscene/Scene.h"

#include <unordered_map>
#include <deque>

using namespace android::base;

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

namespace android {
namespace virtualscene {

LazyInstance<Lock> VirtualSceneManager::mLock = LAZY_INSTANCE_INIT;
VirtualSceneManagerImpl* VirtualSceneManager::mImpl = nullptr;

// Stores settings for the virtual scene.
//
// Access to the instance of this class, sSettings should be guarded by
// VirtualSceneManager::mLock.
class Settings {
public:
    // Defines the setting for a single poster.
    struct PosterSetting {
        std::string mFilename;
        float mSize = 1.0f;
    };

    Settings() = default;

    void parseCmdlineParameter(StringView param) {
        auto it = std::find(param.begin(), param.end(), '=');
        if (it == param.end()) {
            E("%s: Invalid command line parameter '%s', should be "
              "<name>=<filename>",
              __FUNCTION__, std::string(param).c_str());
            return;
        }

        std::string name(param.begin(), it);
        StringView filename(++it, param.end());

        std::string absFilename;
        if (!PathUtils::isAbsolute(filename)) {
            absFilename = PathUtils::join(System::get()->getCurrentDirectory(),
                                          filename);
        } else {
            absFilename = filename;
        }

        if (!System::get()->pathExists(absFilename)) {
            E("%s: Path '%s' does not exist.", __FUNCTION__,
              absFilename.c_str());
            return;
        }

        D("%s: Found poster %s at %s", __FUNCTION__, name.c_str(),
          absFilename.c_str());

        mPosters[name].mFilename = absFilename;
    }

    // Set the poster if it is not already defined.
    void setInitialPoster(const char* posterName, const char* filename) {
        if (mPosters.find(posterName) == mPosters.end()) {
            setPoster(posterName, filename);
        }
    }

    // Set the poster and queue a scene update for the next frame.
    void setPoster(const char* posterName, const char* filename) {
        mPosters[posterName].mFilename = filename ? filename : std::string();
    }

    // Set the poster size.
    void setPosterSize(const char* posterName, float size) {
        mPosters[posterName].mSize = size;
    }

    const std::unordered_map<std::string, PosterSetting>& getPosters() const {
        return mPosters;
    }

private:
    std::unordered_map<std::string, PosterSetting> mPosters;
};

static LazyInstance<Settings> sSettings = LAZY_INSTANCE_INIT;

// All functions in this class must be called under the
// VirtualSceneManager::mLock lock.
class VirtualSceneManagerImpl {
private:
    VirtualSceneManagerImpl(std::unique_ptr<Renderer>&& renderer,
                            std::unique_ptr<Scene>&& scene);

public:
    ~VirtualSceneManagerImpl();

    static std::unique_ptr<VirtualSceneManagerImpl>
    create(const GLESv2Dispatch* gles2, int width, int height);

    int64_t render();

    // Queue an update to a poster filename that should be executed on the
    // render thread.
    void queuePosterUpdate(const char* posterName);

    // Update the poster size.
    void updatePosterSize(const char* posterName, float size);

    // Load a PosterSetting in the scene.
    void loadPosterInternal(const char* posterName,
                            const Settings::PosterSetting& setting);

private:
    std::unique_ptr<Renderer> mRenderer;
    std::unique_ptr<Scene> mScene;

    std::deque<std::string> mPosterFilenameUpdates;
};

VirtualSceneManagerImpl::VirtualSceneManagerImpl(
        std::unique_ptr<Renderer>&& renderer,
        std::unique_ptr<Scene>&& scene)
    : mRenderer(std::move(renderer)), mScene(std::move(scene)) {
    // Load the poster configuration in the scene.
    for (const auto& it : sSettings->getPosters()) {
        loadPosterInternal(it.first.c_str(), it.second);
    }
}

VirtualSceneManagerImpl::~VirtualSceneManagerImpl() {
    skin_winsys_show_virtual_scene_controls(false);
    mScene->releaseResources();
}

std::unique_ptr<VirtualSceneManagerImpl> VirtualSceneManagerImpl::create(
        const GLESv2Dispatch* gles2,
        int width,
        int height) {
    std::unique_ptr<Renderer> renderer = Renderer::create(gles2, width, height);
    if (!renderer) {
        E("VirtualSceneManager renderer failed to construct");
        return nullptr;
    }

    std::unique_ptr<Scene> scene = Scene::create(*renderer.get());
    if (!scene) {
        E("VirtualSceneManager scene failed to load");
        return nullptr;
    }

    skin_winsys_show_virtual_scene_controls(true);

    return std::unique_ptr<VirtualSceneManagerImpl>(
            new VirtualSceneManagerImpl(std::move(renderer), std::move(scene)));
}

int64_t VirtualSceneManagerImpl::render() {
    // Apply any pending updates to the scene.  This must be done on the OpenGL
    // thread.
    const auto& posters = sSettings->getPosters();
    while (!mPosterFilenameUpdates.empty()) {
        const std::string& posterName = mPosterFilenameUpdates.front();
        loadPosterInternal(posterName.c_str(), posters.at(posterName));
        mPosterFilenameUpdates.pop_front();
    }

    const int64_t timestamp = mScene->update();
    mRenderer->render(mScene->getRenderableObjects(),
                      timestamp / 1000000000.0f);
    return timestamp;
}

void VirtualSceneManagerImpl::queuePosterUpdate(const char* posterName) {
    mPosterFilenameUpdates.push_back(posterName);
}

void VirtualSceneManagerImpl::updatePosterSize(const char* posterName,
                                               float size) {
    mScene->updatePosterSize(posterName, size);
}

void VirtualSceneManagerImpl::loadPosterInternal(
        const char* posterName,
        const Settings::PosterSetting& setting) {
    if (setting.mFilename.empty()) {
        // Always render empty posters at 100% size.
        mScene->loadPoster(posterName, nullptr, 1.0f);
    } else {
        mScene->loadPoster(posterName, setting.mFilename.c_str(),
                           setting.mSize);
    }
}

/*******************************************************************************
 *                     VirtualSceneManager API.
 ******************************************************************************/

void VirtualSceneManager::parseCmdline() {
    AutoLock lock(mLock.get());
    if (sSettings.hasInstance()) {
        E("VirtualSceneManager settings already loaded");
        return;
    }

    if (!android_cmdLineOptions) {
        return;
    }

    if (!androidHwConfig_hasVirtualSceneCamera(android_hw) &&
        android_cmdLineOptions->virtualscene_poster) {
        W("[VirtualScene] Poster parameter ignored, virtual scene is not "
          "enabled.");
        return;
    }

    const ParamList* feature = android_cmdLineOptions->virtualscene_poster;
    while (feature) {
        sSettings->parseCmdlineParameter(feature->param);
        feature = feature->next;
    }
}

bool VirtualSceneManager::initialize(const GLESv2Dispatch* gles2,
                                     int width,
                                     int height) {
    AutoLock lock(mLock.get());
    if (mImpl) {
        E("VirtualSceneManager already initialized");
        return false;
    }

    mImpl = VirtualSceneManagerImpl::create(gles2, width, height).release();
    return mImpl != nullptr;
}

void VirtualSceneManager::uninitialize() {
    AutoLock lock(mLock.get());
    if (!mImpl) {
        E("VirtualSceneManager not initialized");
        return;
    }

    // To make sure it's released the same way it was created, attach to a
    // unique_ptr and let that release it.
    std::unique_ptr<VirtualSceneManagerImpl> stateReleaser(mImpl);
    mImpl = nullptr;
}

int64_t VirtualSceneManager::render() {
    AutoLock lock(mLock.get());
    if (!mImpl) {
        E("VirtualSceneManager not initialized");
        return 0L;
    }

    return mImpl->render();
}

void VirtualSceneManager::setInitialPoster(const char* posterName,
                                           const char* filename) {
    AutoLock lock(mLock.get());
    sSettings->setInitialPoster(posterName, filename);

    // If the scene is active, it will update the poster in the next render()
    // invocation.
    if (mImpl) {
        mImpl->queuePosterUpdate(posterName);
    }
}

bool VirtualSceneManager::loadPoster(const char* posterName,
                                     const char* filename) {
    AutoLock lock(mLock.get());
    sSettings->setPoster(posterName, filename);

    // If the scene is active, it will update the poster in the next render()
    // invocation.
    if (mImpl) {
        mImpl->queuePosterUpdate(posterName);
    }

    return true;
}

void VirtualSceneManager::enumeratePosters(void* context,
                                           EnumeratePostersCallback callback) {
    AutoLock lock(mLock.get());

    for (const auto& it : sSettings->getPosters()) {
        callback(context, it.first.c_str(),
                 it.second.mFilename.empty() ? nullptr
                                             : it.second.mFilename.c_str(),
                 it.second.mSize);
    }
}

void VirtualSceneManager::setPosterSize(const char* posterName, float size) {
    AutoLock lock(mLock.get());
    sSettings->setPosterSize(posterName, size);

    // Updating the poster size can be done on any thread, update it now.
    if (mImpl) {
        mImpl->updatePosterSize(posterName, size);
    }
}

}  // namespace virtualscene
}  // namespace android
