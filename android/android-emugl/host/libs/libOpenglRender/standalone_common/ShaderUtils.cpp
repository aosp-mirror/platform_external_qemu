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

#include "ShaderUtils.h"

#include "android/base/files/PathUtils.h"
#include "android/base/files/ScopedFd.h"
#include "android/base/memory/ScopedPtr.h"
#include "android/base/misc/FileUtils.h"
#include "android/base/Optional.h"
#include "android/base/system/System.h"
#include "android/utils/path.h"
#include "android/utils/tempfile.h"

#include "emugl/common/OpenGLDispatchLoader.h"

#include <fstream>
#include <vector>

#include <fcntl.h>
#include <stdio.h>

using android::base::Optional;
using android::base::pj;
using android::base::System;

namespace emugl {

GLuint compileShader(GLenum shaderType, const char* src) {
    auto gl = LazyLoadedGLESv2Dispatch::get();

    GLuint shader = gl->glCreateShader(shaderType);
    gl->glShaderSource(shader, 1, (const GLchar* const*)&src, nullptr);
    gl->glCompileShader(shader);

    GLint compileStatus;
    gl->glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);

    if (compileStatus != GL_TRUE) {
        GLsizei infoLogLength = 0;
        gl->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<char> infoLog(infoLogLength + 1, 0);
        gl->glGetShaderInfoLog(shader, infoLogLength, nullptr, &infoLog[0]);
        fprintf(stderr, "%s: fail to compile. infolog %s\n", __func__, &infoLog[0]);
    }

    return shader;
}

GLint compileAndLinkShaderProgram(const char* vshaderSrc, const char* fshaderSrc) {
    auto gl = LazyLoadedGLESv2Dispatch::get();

    GLuint vshader = compileShader(GL_VERTEX_SHADER, vshaderSrc);
    GLuint fshader = compileShader(GL_FRAGMENT_SHADER, fshaderSrc);

    GLuint program = gl->glCreateProgram();
    gl->glAttachShader(program, vshader);
    gl->glAttachShader(program, fshader);
    gl->glLinkProgram(program);

    GLint linkStatus;
    gl->glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

    gl->glClearColor(0.0f, 0.0f, 1.0f, 0.0f);

    if (linkStatus != GL_TRUE) {
        GLsizei infoLogLength = 0;
        gl->glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::vector<char> infoLog(infoLogLength + 1, 0);
        gl->glGetProgramInfoLog(program, infoLogLength, nullptr, &infoLog[0]);

        fprintf(stderr, "%s: fail to link program. infolog: %s\n", __func__,
                &infoLog[0]);
    }

    return program;
}

static Optional<std::string> getSpirvCompilerPath() {
#ifdef _WIN32
    std::string programName = "glslangValidator.exe";
#else
    std::string programName = "glslangValidator";
#endif

    auto programDirRelativePath =
        pj(System::get()->getProgramDirectory(),
           "lib64", "vulkan", "tools", programName);

    if (path_exists(programDirRelativePath.c_str())) {
        return programDirRelativePath;
    }

    auto launcherDirRelativePath =
        pj(System::get()->getLauncherDirectory(),
           "lib64", "vulkan", programName);
    
    if (path_exists(launcherDirRelativePath.c_str())) {
        return launcherDirRelativePath;
    }

    fprintf(stderr, "%s: spirv compiler does not exist\n", __func__);
    return {};
}

Optional<std::string> compileSpirvFromGLSL(const std::string& shaderType,
                                           const std::string& src) {
    auto spvCompilerPath = getSpirvCompilerPath();
    
    if (!spvCompilerPath) return {};

    const auto glslFile = android::base::makeCustomScopedPtr(
            tempfile_create(), tempfile_close);

    const auto spvFile = android::base::makeCustomScopedPtr(
            tempfile_create(), tempfile_close);

    auto glslPath = tempfile_path(glslFile.get());
    auto spvPath = tempfile_path(spvFile.get());

    auto glslFd = android::base::ScopedFd(open(glslPath, O_RDWR));
    if (!glslFd.valid()) { return {}; }

    android::writeStringToFile(glslFd.get(), src);

    std::vector<std::string> args =
        { *spvCompilerPath, glslPath, "-V", "-S", shaderType, "-o", spvPath };

    auto runRes = System::get()->runCommandWithResult(args);

    if (!runRes) {
        fprintf(stderr, "%s: failed to compile\n", __func__);
        return {};
    }

    fprintf(stderr, "%s: run result: %s\n", __func__, runRes->c_str());

    auto res = android::readFileIntoString(spvPath);

    if (res) {
        fprintf(stderr, "%s: got %zu bytes:\n", __func__, res->size());
    } else {
        fprintf(stderr, "%s: failed to read file into string\n", __func__);
    }

    return res;
}

Optional<std::vector<char> > readSpirv(const char* path) {
    std::ifstream in(path, std::ios::ate | std::ios::binary);

    if (!in) return {};

    size_t fileSize = (size_t)in.tellg();
    std::vector<char> buffer(fileSize);

    in.seekg(0);
    in.read(buffer.data(), fileSize);

    return buffer;
}




} // namespace emugl
