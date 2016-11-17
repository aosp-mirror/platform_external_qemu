// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "ANGLEShaderParser.h"

#include "android/base/synchronization/Lock.h"

#include <string>

#include <GLSLANG/ShaderLang.h>

namespace ANGLEShaderParser {

ShBuiltInResources kResources;
bool kInitialized = false;

ShHandle kVertCompiler = nullptr;
ShHandle kFragCompiler = nullptr;

android::base::Lock kCompilerLock;

void initializeResources() {
    ShInitBuiltInResources(&kResources);

    // The following are set to pass:
    // dEQP-GLES2.functional.shaders.builtin_variable.*
    // On a Linux machine with an Nvidia Quadro K2200
    kResources.MaxVertexAttribs = 16; // Defaulted to 8
    kResources.MaxVertexUniformVectors = 1024; // Defaulted to 128
    kResources.MaxVaryingVectors = 31; // Defaulted to 8
    kResources.MaxVertexTextureImageUnits = 32; // Defaulted to 0
    kResources.MaxCombinedTextureImageUnits = 32; // Defaulted to 8
    kResources.MaxTextureImageUnits = 32; // Defaulted to 8
    kResources.MaxFragmentUniformVectors = 1024; // Defaulted to 16

    kResources.MaxDrawBuffers = 1;
    kResources.MaxDualSourceDrawBuffers = 1;

    kResources.OES_standard_derivatives = 0;
    kResources.OES_EGL_image_external = 0;

    kResources.FragmentPrecisionHigh = 1;
}

ShHandle createShaderCompiler(GLenum shaderType) {
    ShHandle handle = ShConstructCompiler(shaderType,
                                          SH_GLES2_SPEC,
                                          SH_GLSL_COMPATIBILITY_OUTPUT,
                                          &kResources);
    return handle;
}

bool globalInitialize() {
    if (!ShInitialize()) {
        fprintf(stderr, "Global ANGLE shader compiler initialzation failed.\n");
        return false;
    }
    initializeResources();

    kVertCompiler = createShaderCompiler(GL_VERTEX_SHADER);
    if (!kVertCompiler) {
        fprintf(stderr, "Failed to initialize ANGLE vertex shader compiler.\n");
        return false;
    }
    kFragCompiler = createShaderCompiler(GL_FRAGMENT_SHADER);
    if (!kFragCompiler) {
        fprintf(stderr, "Failed to initialize ANGLE fragment shader compiler.\n");
        return false;
    }

    kInitialized = true;
    return true;
}

bool translate(const char* src, GLenum shaderType,
                                std::string* outInfolog,
                                std::string* outObjCode) {
    if (!kInitialized) {
        return false;
    }

    // ANGLE may crash if multiple RenderThreads attempt to compile shaders
    // at the same time.
    android::base::AutoLock autolock(kCompilerLock);

    ShHandle compilerHandle = (shaderType == GL_VERTEX_SHADER ?
                                   kVertCompiler : kFragCompiler);
    if (!compilerHandle) {
        return false;
    }

    // Pass in the entire src as 1 string, ask for compiled GLSL object code
    // to be saved.
    int res = ShCompile(compilerHandle, &src, 1, SH_OBJECT_CODE);

    // The compilers return references that may not be valid in the future,
    // and we manually clear them immediately anyway.
    *outInfolog = std::string(ShGetInfoLog(compilerHandle));
    *outObjCode = std::string(ShGetObjectCode(compilerHandle));
    ShClearResults(compilerHandle);

    return res;
}

}
