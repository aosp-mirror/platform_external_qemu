// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include <GLES2/gl2.h>

#include "android/base/Optional.h"

#include <string>
#include <vector>

namespace emugl {

GLuint compileShader(GLenum shaderType, const char* src);
GLint compileAndLinkShaderProgram(const char* vshaderSrc, const char* fshaderSrc);

android::base::Optional<std::string> compileSpirvFromGLSL(
        const std::string& shaderType,
        const std::string& src);

android::base::Optional<std::vector<char>> readSpirv(const char* path);

} // namespace emugl
