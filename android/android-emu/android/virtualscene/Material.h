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
 * Defines a Material container type used by the Virtual Scene Renderer.
 */

#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "android/base/Compiler.h"

namespace android {
namespace virtualscene {

// Forward declarations.
class Renderer;

class Material {
    DISALLOW_COPY_AND_ASSIGN(Material);
    friend class Renderer;

public:
    ~Material();

private:
    Material(const GLESv2Dispatch* gles2, GLuint program);

    const GLESv2Dispatch* mGles2;
    const GLuint mProgram;

    GLint mPositionLocation = -1;
    GLint mUvLocation = -1;
    GLint mTexSamplerLocation = -1;
    GLint mMvpLocation = -1;
    GLint mResolutionLocation = -1;
};

}  // namespace virtualscene
}  // namespace android
