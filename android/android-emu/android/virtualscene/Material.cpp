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

#include "android/virtualscene/Material.h"

namespace android {
namespace virtualscene {

Material::Material(const GLESv2Dispatch* gles2, GLuint program)
    : mGles2(gles2), mProgram(program) {}

Material::~Material() {
    // Clean up outstanding OpenGL handles; releasing a 0 handle no-ops.
    mGles2->glDeleteProgram(mProgram);
}

}  // namespace virtualscene
}  // namespace android
