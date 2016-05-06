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

#include <string>

ShBuiltInResources ANGLEShaderParser::kResources;
ShHandle ANGLEShaderParser::kVertCompiler;
ShHandle ANGLEShaderParser::kFragCompiler;
bool ANGLEShaderParser::kInitialized;

bool ANGLEShaderParser::initialize() {
    if (!kInitialized) {
        if (!ShInitialize()) {
            fprintf(stdout, "Shader parsed failed to initialize!\n");
            return false;
        }
        initializeResources();
        kVertCompiler = ShConstructCompiler(GL_VERTEX_SHADER,
                                            SH_GLES2_SPEC,
                                            kGlslVersion,
                                            &kResources);
        if (!kVertCompiler) {
            fprintf(stdout, "Could not create vertex shader compiler!\n");
            return false;
        }

        kFragCompiler = ShConstructCompiler(GL_FRAGMENT_SHADER,
                                            SH_GLES2_SPEC,
                                            kGlslVersion,
                                            &kResources);
        if (!kFragCompiler) {
            fprintf(stdout, "Could not create fragment shader compiler!\n");
            return false;
        }

        kInitialized = true;
        return true;
    }
    return false;
}

void ANGLEShaderParser::initializeResources() {
    ShInitBuiltInResources(&kResources);

    // The following are set to pass:
    // dEQP-GLES2.functional.shaders.builtin_variable.*
    kResources.MaxVertexAttribs = 16; // Defaulted to 8
    kResources.MaxVertexUniformVectors = 1024; // Defaulted to 128
    kResources.MaxVaryingVectors = 31; // Defaulted to 8
    kResources.MaxVertexTextureImageUnits = 32; // Defaulted to 0
    kResources.MaxCombinedTextureImageUnits = 16; // Defaulted to 8
    kResources.MaxTextureImageUnits = 32; // Defaulted to 8
    kResources.MaxFragmentUniformVectors = 1024; // Defaulted to 16

    kResources.MaxDrawBuffers = 1;
    kResources.MaxDualSourceDrawBuffers = 1;

    kResources.OES_standard_derivatives = 0;
    kResources.OES_EGL_image_external = 0;

    kResources.FragmentPrecisionHigh = 1;
}

bool ANGLEShaderParser::translate(const char* src, GLenum shaderType,
                                std::string* outInfolog,
                                std::string* outObjCode) {
    if (!kInitialized) {
        fprintf(stdout, "ANGLE shader parser was never initialized.\n");
        return false;
    }
    // TODO: worry about other kinds of shaders after adding GLESv3 support,
    // such as geometry shaders.
    ShHandle compilerHandle = (shaderType == GL_VERTEX_SHADER ?
                                   kVertCompiler : kFragCompiler);

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
