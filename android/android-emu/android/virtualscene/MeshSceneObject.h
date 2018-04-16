// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

/*
 * Defines MeshSceneObject, which represents a SceneObject loaded from an .obj
 * file.
 */

#include "android/base/Compiler.h"
#include "android/utils/compiler.h"
#include "android/virtualscene/SceneObject.h"

namespace android {
namespace virtualscene {

class MeshSceneObject : public SceneObject {
    DISALLOW_COPY_AND_ASSIGN(MeshSceneObject);

protected:
    MeshSceneObject(Renderer& renderer);

public:
    // Loads an object mesh from an .obj file.
    //
    // |renderer| - Renderer context.
    // |filename| - Filename to load.
    //
    // Returns a MeshSceneObject instance if the object could be loaded or null
    // if there was an error.
    static std::unique_ptr<MeshSceneObject> load(Renderer& renderer,
                                                 const char* filename);
};

}  // namespace virtualscene
}  // namespace android
