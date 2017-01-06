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

#define SH_GLES31_SPEC 0x8B88
#define GL_COMPUTE_SHADER 0x91B9

namespace ANGLEShaderParser {

ShBuiltInResources kResources;
bool kInitialized = false;

ShHandle kVertCompilerCompat = nullptr;
ShHandle kFragCompilerCompat = nullptr;

ShHandle kVertCompilerES30 = nullptr;
ShHandle kFragCompilerES30 = nullptr;

ShHandle kVertCompilerES31 = nullptr;
ShHandle kFragCompilerES31 = nullptr;
ShHandle kComputeCompilerES31 = nullptr;

android::base::Lock kCompilerLock;

void initializeResources(
            int attribs,
            int uniformVectors,
            int varyingVectors,
            int vertexTextureImageUnits,
            int combinedTexImageUnits,
            int textureImageUnits,
            int fragmentUniformVectors,
            int drawBuffers,
            int fragmentPrecisionHigh,
            int vertexOutputComponents,
            int fragmentInputComponents,
            int minProgramTexelOffset,
            int maxProgramTexelOffset,
            int maxDualSourceDrawBuffers) {
    ShInitBuiltInResources(&kResources);

    kResources.MaxVertexAttribs = attribs; // Defaulted to 8
    kResources.MaxVertexUniformVectors = uniformVectors; // Defaulted to 128
    kResources.MaxVaryingVectors = varyingVectors; // Defaulted to 8
    kResources.MaxVertexTextureImageUnits = vertexTextureImageUnits; // Defaulted to 0
    kResources.MaxCombinedTextureImageUnits = combinedTexImageUnits; // Defaulted to 8
    kResources.MaxTextureImageUnits = textureImageUnits; // Defaulted to 8
    kResources.MaxFragmentUniformVectors = fragmentUniformVectors; // Defaulted to 16

    kResources.MaxDrawBuffers = drawBuffers;
    kResources.FragmentPrecisionHigh = fragmentPrecisionHigh;

    kResources.MaxVertexOutputVectors = vertexOutputComponents / 4;
    kResources.MaxFragmentInputVectors = fragmentInputComponents / 4;
    kResources.MinProgramTexelOffset = minProgramTexelOffset;
    kResources.MaxProgramTexelOffset = maxProgramTexelOffset;

    kResources.MaxDualSourceDrawBuffers = maxDualSourceDrawBuffers;

    kResources.OES_standard_derivatives = 0;
    kResources.OES_EGL_image_external = 0;
}

ShHandle createShaderCompiler(GLenum shaderType, int esSpec, ShShaderOutput glslout) {
    ShHandle handle = ShConstructCompiler(shaderType,
                                          (ShShaderSpec)esSpec,
                                          glslout,
                                          &kResources);
    return handle;
}

bool globalInitialize(
            int attribs,
            int uniformVectors,
            int varyingVectors,
            int vertexTextureImageUnits,
            int combinedTexImageUnits,
            int textureImageUnits,
            int fragmentUniformVectors,
            int drawBuffers,
            int fragmentPrecisionHigh,
            int vertexOutputComponents,
            int fragmentInputComponents,
            int minProgramTexelOffset,
            int maxProgramTexelOffset,
            int maxDualSourceDrawBuffers) {
    if (!ShInitialize()) {
        fprintf(stderr, "Global ANGLE shader compiler initialzation failed.\n");
        return false;
    }
    initializeResources(
            attribs,
            uniformVectors,
            varyingVectors,
            vertexTextureImageUnits,
            combinedTexImageUnits,
            textureImageUnits,
            fragmentUniformVectors,
            drawBuffers,
            fragmentPrecisionHigh,
            vertexOutputComponents,
            fragmentInputComponents,
            minProgramTexelOffset,
            maxProgramTexelOffset,
            maxDualSourceDrawBuffers);

    kVertCompilerCompat = createShaderCompiler(GL_VERTEX_SHADER, SH_GLES3_SPEC, SH_GLSL_COMPATIBILITY_OUTPUT);
    if (!kVertCompilerCompat) {
        fprintf(stderr, "Failed to initialize ANGLE Compat vertex shader compiler.\n");
        return false;
    }

    kFragCompilerCompat = createShaderCompiler(GL_FRAGMENT_SHADER, SH_GLES3_SPEC, SH_GLSL_COMPATIBILITY_OUTPUT);
    if (!kFragCompilerCompat) {
        fprintf(stderr, "Failed to initialize ANGLE Compat fragment shader compiler.\n");
        return false;
    }

    kVertCompilerES30 = createShaderCompiler(GL_VERTEX_SHADER, SH_GLES3_SPEC, SH_GLSL_150_CORE_OUTPUT);
    if (!kVertCompilerES30) {
        fprintf(stderr, "Failed to initialize ANGLE ES 3.0 vertex shader compiler.\n");
        return false;
    }

    kFragCompilerES30 = createShaderCompiler(GL_FRAGMENT_SHADER, SH_GLES3_SPEC, SH_GLSL_150_CORE_OUTPUT);
    if (!kFragCompilerES30) {
        fprintf(stderr, "Failed to initialize ANGLE ES 3.0 fragment shader compiler.\n");
        return false;
    }

    kVertCompilerES31 = createShaderCompiler(GL_VERTEX_SHADER, SH_GLES31_SPEC, SH_GLSL_430_CORE_OUTPUT);
    if (!kVertCompilerES31) {
        fprintf(stderr, "Failed to initialize ANGLE ES 3.1 vertex shader compiler.\n");
        return false;
    }

    kFragCompilerES31 = createShaderCompiler(GL_FRAGMENT_SHADER, SH_GLES31_SPEC, SH_GLSL_430_CORE_OUTPUT);
    if (!kFragCompilerES31) {
        fprintf(stderr, "Failed to initialize ANGLE ES 3.1 fragment shader compiler.\n");
        return false;
    }
    kComputeCompilerES31 = createShaderCompiler(GL_COMPUTE_SHADER, SH_GLES31_SPEC, SH_GLSL_430_CORE_OUTPUT);
    if (!kComputeCompilerES31) {
        fprintf(stderr, "Failed to initialize ANGLE ES 3.1 compute shader compiler.\n");
        return false;
    }
    kInitialized = true;
    return true;
}

bool translate(int glesMajorVersion,
               int glesMinorVersion,
               const char* src, GLenum shaderType,
                                std::string* outInfolog,
                                std::string* outObjCode) {
    if (!kInitialized) {
        return false;
    }

    // ANGLE may crash if multiple RenderThreads attempt to compile shaders
    // at the same time.
    android::base::AutoLock autolock(kCompilerLock);

    ShHandle compilerHandle = (shaderType == GL_VERTEX_SHADER ?
                                   kVertCompilerCompat : kFragCompilerCompat);

    if (glesMajorVersion == 3 && glesMinorVersion == 0) {
        compilerHandle =
            (shaderType == GL_VERTEX_SHADER ? kVertCompilerES30 : kFragCompilerES30);

    } else if (glesMajorVersion == 3 && glesMinorVersion == 1) {
        fprintf(stderr, "%s: using 3.1 ANGLE shader translator\n", __func__);
        if (shaderType == GL_COMPUTE_SHADER)
            fprintf(stderr, "%s: is compute shader as well\n", __func__);
        switch (shaderType) {
            case GL_VERTEX_SHADER:
                compilerHandle = kVertCompilerES31;
                break;
            case GL_FRAGMENT_SHADER:
                compilerHandle = kFragCompilerES31;
                break;
            case GL_COMPUTE_SHADER:
                compilerHandle = kComputeCompilerES31;
                break;
            default:
                compilerHandle = 0;
                break;
        }
    }

    if (!compilerHandle) {
        fprintf(stderr, "%s: no compiler handle\n", __FUNCTION__);
        return false;
    }

    // Pass in the entire src as 1 string, ask for compiled GLSL object code
    // to be saved.
    if (shaderType == GL_COMPUTE_SHADER) {
        fprintf(stderr, "%s: **** Compute shader INPUT ****\n%s\n", __FUNCTION__, src);
    }
    int res = ShCompile(compilerHandle, &src, 1, SH_OBJECT_CODE);

    // The compilers return references that may not be valid in the future,
    // and we manually clear them immediately anyway.
    *outInfolog = std::string(ShGetInfoLog(compilerHandle));
    *outObjCode = std::string(ShGetObjectCode(compilerHandle));
    if (shaderType == GL_COMPUTE_SHADER) {
        fprintf(stderr, "%s: **** Compute shader INFOLOG ****\n%s\n", __FUNCTION__, outInfolog->c_str());
        fprintf(stderr, "%s: **** Compute shader OUTPUT ****\n%s\n", __FUNCTION__, outObjCode->c_str());
    }
    ShClearResults(compilerHandle);


    return res;
}

}
