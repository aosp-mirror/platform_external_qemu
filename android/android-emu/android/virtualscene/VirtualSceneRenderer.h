/*
 * Copyright (C) 2011 The Android Open Source Project
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
 * Defines the Virtual Scene Renderer, used by the Virtual Scene Camera
 */

#include "android/utils/compiler.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

#include <mutex>

class VirtualSceneRendererImpl;

class VirtualSceneRenderer {
public:
    static void initialize(const GLESv2Dispatch* gles2);
    static void uninitialize();
    static void render();

private:
    static std::mutex lock_;
    static VirtualSceneRendererImpl* impl_;
};
