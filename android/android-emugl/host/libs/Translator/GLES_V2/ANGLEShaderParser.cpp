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

#include <dlfcn.h>

#include <map>
#include <string>

#define GL_COMPUTE_SHADER 0x91B9

namespace ANGLEShaderParser {

ST_BuiltInResources kResources;
bool kInitialized = false;

struct ShaderSpecKey {
    GLenum shaderType;
    int esslVersion;
};

static ST_ShaderSpec sInputSpecForVersion(int esslVersion) {
    switch (esslVersion) {
        case 100:
            return ST_GLES2_SPEC;
        case 300:
            return ST_GLES3_SPEC;
        case 310:
            return ST_GLES3_1_SPEC;
    }
    return ST_GLES3_1_SPEC;
}

static ST_ShaderOutput sOutputSpecForVersion(bool coreProfileHost, int esslVersion) {
    switch (esslVersion) {
        case 100:
            if (coreProfileHost) {
                return ST_GLSL_330_CORE_OUTPUT;
            } else {
                return ST_GLSL_COMPATIBILITY_OUTPUT;
            }
        case 300:
            if (coreProfileHost) {
                return ST_GLSL_330_CORE_OUTPUT;
            } else {
                return ST_GLSL_150_CORE_OUTPUT;
            }
        case 310:
            return ST_GLSL_430_CORE_OUTPUT;
    }
    return ST_GLSL_430_CORE_OUTPUT;
}

struct ShaderSpecKeyCompare {
    bool operator() (const ShaderSpecKey& a,
                     const ShaderSpecKey& b) const {
        if (a.shaderType != b.shaderType)
            return a.shaderType < b.shaderType;
        if (a.esslVersion != b.esslVersion)
            return a.esslVersion < b.esslVersion;
        return false;
    }
};

typedef std::map<ShaderSpecKey, ST_Handle, ShaderSpecKeyCompare> ShaderCompilerMap;
static ShaderCompilerMap sCompilerMap;

static ST_Handle getShaderCompiler(bool coreProfileHost, ShaderSpecKey key) {
    if (sCompilerMap.find(key) == sCompilerMap.end()) {
        return (ST_Handle)nullptr;
    }
    return sCompilerMap[key];
}

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
            int maxDualSourceDrawBuffers,
            bool shaderFramebufferFetch) {
    STGenerateResources(&kResources);

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

    kResources.OES_standard_derivatives = 1;
    kResources.OES_EGL_image_external = 0;
    kResources.EXT_gpu_shader5 = 1;
    kResources.EXT_shader_framebuffer_fetch = shaderFramebufferFetch ? 1 : 0;
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
            int maxDualSourceDrawBuffers,
            bool shaderFramebufferFetch) {



    STInitialize();

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
            maxDualSourceDrawBuffers,
            shaderFramebufferFetch);

    kInitialized = true;
    return true;
}

template <class T>
static std::vector<T> convertArrayToVecWithCopy(
    unsigned int count, const T* pItems, T (*copyFunc)(const T*)) {
    std::vector<T> res;
    for (uint32_t i = 0; i < count; ++i) {
        res.push_back(copyFunc(pItems + i));
    }
    return res;
}

static void getShaderLinkInfo(int esslVersion,
                              const ST_ShaderCompileResult* compileResult,
                              ShaderLinkInfo* linkInfo) {
    linkInfo->esslVersion = esslVersion;

    for (uint32_t i = 0; i < compileResult->nameHashingMap->entryCount; ++i) {
        linkInfo->nameMap[compileResult->nameHashingMap->ppUserNames[i]] =
            compileResult->nameHashingMap->ppCompiledNames[i];
    }

    for (const auto& elt : linkInfo->nameMap) {
        linkInfo->nameMapReverse[elt.second] = elt.first;
    }

    linkInfo->uniforms = convertArrayToVecWithCopy(
        compileResult->uniformsCount,
        compileResult->pUniforms,
        STCopyVariable);

    std::vector<ST_ShaderVariable> inputVaryings =
        convertArrayToVecWithCopy(
            compileResult->inputVaryingsCount, compileResult->pInputVaryings,
            STCopyVariable);
    std::vector<ST_ShaderVariable> outputVaryings =
        convertArrayToVecWithCopy(
            compileResult->outputVaryingsCount, compileResult->pOutputVaryings,
            STCopyVariable);

    linkInfo->varyings.clear();
    linkInfo->varyings.insert(
        linkInfo->varyings.begin(),
        inputVaryings.begin(),
        inputVaryings.end());
    linkInfo->varyings.insert(
        linkInfo->varyings.begin(),
        outputVaryings.begin(),
        outputVaryings.end());

    linkInfo->attributes =
        convertArrayToVecWithCopy(
            compileResult->allAttributesCount,
            compileResult->pAllAttributes,
            STCopyVariable);

    linkInfo->outputVars =
        convertArrayToVecWithCopy(
            compileResult->activeOutputVariablesCount,
            compileResult->pActiveOutputVariables,
            STCopyVariable);

    linkInfo->interfaceBlocks =
        convertArrayToVecWithCopy(
            compileResult->uniformBlocksCount,
            compileResult->pUniformBlocks,
            STCopyInterfaceBlock);
    // todo: split to uniform and ssbo
}

static int detectShaderESSLVersion(const char* const* strings) {
    // Just look at the first line of the first string for now
    const char* pos = strings[0];
    const char* linePos = strstr(pos, "\n");
    const char* versionPos = strstr(pos, "#version");
    if (!linePos || !versionPos) {
        // default to ESSL 100
        return 100;
    }

    const char* version_end = versionPos + strlen("#version");
    int wantedESSLVersion;
    sscanf(version_end, " %d", &wantedESSLVersion);
    return wantedESSLVersion;
}

bool translate(bool hostUsesCoreProfile,
               const char* src,
               GLenum shaderType,
               std::string* outInfolog,
               std::string* outObjCode,
               ShaderLinkInfo* outShaderLinkInfo) {
    int esslVersion = detectShaderESSLVersion(&src);
    // ANGLE may crash if multiple RenderThreads attempt to compile shaders
    // at the same time.
    android::base::AutoLock autolock(kCompilerLock);

    ShaderSpecKey key;
    key.shaderType = shaderType;
    key.esslVersion = esslVersion;

    ST_ShaderCompileInfo ci = {
        (ST_Handle)getShaderCompiler(hostUsesCoreProfile, key),
        shaderType,
        sInputSpecForVersion(esslVersion),
        sOutputSpecForVersion(hostUsesCoreProfile, esslVersion),
        ST_OBJECT_CODE | ST_VARIABLES,
        &kResources,
        src,
    };

    ST_ShaderCompileResult* res = nullptr;

    STCompileAndResolve(&ci, &res);

    sCompilerMap[key] = res->outputHandle;
    *outInfolog = std::string(res->infoLog);
    *outObjCode = std::string(res->translatedSource);

    if (outShaderLinkInfo) getShaderLinkInfo(esslVersion, res, outShaderLinkInfo);

    bool ret = res->compileStatus == 1;

    STFreeShaderResolveState(res);
    return ret;
}

} // namespace ANGLEShaderParser
