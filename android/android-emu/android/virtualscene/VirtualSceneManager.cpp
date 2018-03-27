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

#include "android/base/files/PathUtils.h"
#include "android/cmdline-option.h"
#include "android/globals.h"
#include "android/skin/winsys.h"
#include "android/utils/debug.h"
#include "android/virtualscene/Renderer.h"
#include "android/virtualscene/Scene.h"

#include <unordered_map>

using namespace android::base;

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

namespace android {
namespace virtualscene {

android::base::LazyInstance<android::base::Lock> VirtualSceneManager::mLock =
        LAZY_INSTANCE_INIT;
Renderer* VirtualSceneManager::mRenderer = nullptr;
Scene* VirtualSceneManager::mScene = nullptr;

class Settings {
public:
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

        mPosters[name] = absFilename;
    }

    void setPoster(const char* posterName, const char* filename) {
        mPosters[posterName] = filename;
    }

private:
    std::unordered_map<std::string, std::string> mPosters;
};

static LazyInstance<Settings> sSettings = LAZY_INSTANCE_INIT;

void VirtualSceneManager::parseCmdline() {
    AutoLock lock(mLock.get());
    if (sSettings.hasInstance()) {
        E("VirtualSceneManager settings already loaded");
        return;
    }

    if (!android_cmdLineOptions) {
        return;
    }

    const bool isVirtualScene =
            !strcmp(android_hw->hw_camera_back, "virtualscene");
    if (!isVirtualScene && android_cmdLineOptions->virtualscene_poster) {
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
    if (mRenderer || mScene) {
        E("VirtualSceneManager already initialized");
        return false;
    }

    std::unique_ptr<Renderer> renderer = Renderer::create(gles2, width, height);
    if (!renderer) {
        E("VirtualSceneManager renderer failed to construct");
        return false;
    }

    std::unique_ptr<Scene> scene = Scene::create(*renderer.get());
    if (!scene) {
        E("VirtualSceneManager scene failed to load");
        return false;
    }

    skin_winsys_show_virtual_scene_controls(true);

    // Store the raw pointers instead of the unique_ptr wrapper to prevent
    // unintented side-effects on process shutdown.
    mRenderer = renderer.release();
    mScene = scene.release();

    return true;
}

void VirtualSceneManager::uninitialize() {
    AutoLock lock(mLock.get());
    if (!mRenderer || !mScene) {
        E("VirtualSceneManager not initialized");
        return;
    }

    skin_winsys_show_virtual_scene_controls(false);

    mScene->releaseSceneObjects(*mRenderer);
    delete mScene;
    mScene = nullptr;

    delete mRenderer;
    mRenderer = nullptr;
}

int64_t VirtualSceneManager::render() {
    AutoLock lock(mLock.get());
    if (!mRenderer || !mScene) {
        E("VirtualSceneManager not initialized");
        return 0L;
    }

    const int64_t timestamp = mScene->update();
    mRenderer->render(mScene->getRenderableObjects(),
            timestamp / 1000000000.0f);
    return timestamp;
}

}  // namespace virtualscene
}  // namespace android
