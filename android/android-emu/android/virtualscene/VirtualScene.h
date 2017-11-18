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
 * Defines the Virtual Scene, used by the Virtual Scene Camera
 */

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/base/memory/LazyInstance.h"
#include "android/base/synchronization/Lock.h"
#include "android/utils/compiler.h"

namespace android {
namespace virtualscene {

// Forward declaration.
class VirtualSceneRenderer;

class VirtualScene {
public:
    // Initialize virtual scene rendering. Callers must have an active EGL
    // context.
    // |gles2| - Pointer to GLESv2Dispatch, must be non-null.
    //
    // Returns true if initialization succeeded.
    static bool initialize(const GLESv2Dispatch* gles2);

    // Uninitialize virtual scene rendering, may be called on any thread, but
    // the same EGL context that was active when initialize() was called must be
    // active. This function modifies the GL state, callers must be resilient to
    // that.
    static void uninitialize();

    // Render the virtual scene to the currently bound render target. This may
    // be called on any thread, but the same EGL context that was active when
    // initialize() was called must be active. This function modifies the GL
    // state, callers must be resilient to that.
    static void render();

private:
    static android::base::LazyInstance<android::base::Lock> mLock;
    static VirtualSceneRenderer* mImpl;
};

}  // namespace virtualscene
}  // namespace android
