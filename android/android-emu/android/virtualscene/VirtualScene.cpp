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

#include "android/virtualscene/VirtualScene.h"

#include "android/skin/winsys.h"
#include "android/utils/debug.h"
#include "android/virtualscene/Renderer.h"

using namespace android::base;

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(virtualscene, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(virtualscene)

namespace android {
namespace virtualscene {

android::base::LazyInstance<android::base::Lock> VirtualScene::mLock =
        LAZY_INSTANCE_INIT;
Renderer* VirtualScene::mImpl = nullptr;

bool VirtualScene::initialize(const GLESv2Dispatch* gles2) {
    AutoLock lock(mLock.get());
    if (mImpl) {
        E("VirtualScene already initialized");
        return false;
    }

    std::unique_ptr<Renderer> renderer = Renderer::create(gles2);
    if (!renderer) {
        E("VirtualScene failed to construct");
        return false;
    }

    skin_winsys_show_virtual_scene_controls(true);

    // Store the raw pointer instead of the unique_ptr wrapper to prevent
    // unintented side-effects on process shutdown.
    mImpl = renderer.release();

    return true;
}

void VirtualScene::uninitialize() {
    AutoLock lock(mLock.get());
    if (!mImpl) {
        E("VirtualScene not initialized");
        return;
    }

    skin_winsys_show_virtual_scene_controls(false);

    delete mImpl;
    mImpl = nullptr;
}

void VirtualScene::render() {
    AutoLock lock(mLock.get());
    if (!mImpl) {
        E("VirtualScene not initialized");
        return;
    }

    mImpl->render();
}

}  // namespace virtualscene
}  // namespace android
