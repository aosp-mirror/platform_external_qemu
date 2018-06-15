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
#include "android/base/memory/LazyInstance.h"

#include "emugl/common/shared_library.h"

#include <map>
#include <string>

namespace ANGLEShaderParser {

ShBuiltInResources kResources;
bool kInitialized = false;

class STLibrary {
public:

    STLibrary() {
        static const char kLibName[] =
                "libShaderTranslator.dylib";
        char error[256];
        lib = emugl::SharedLibrary::open(kLibName, error, sizeof(error));

        if (!lib) {
            fprintf(stderr, "%s: failed to get shader translator lib %s [%s]\n", __func__,
                    kLibName, error);
        }

#define DEFINE_ST_DISPATCH_FIELD(name) \
    name##_t name;

#define FIND_ST_SYMBOL(name) \
        this->name = (name##_t)lib->findSymbol(#name); \
        fprintf(stderr, "%s: %s -> %p\n", __func__, #name, this->name);

        LIST_ST_FUNCS(FIND_ST_SYMBOL);

        this->STInitialize();
    }

    emugl::SharedLibrary* lib = nullptr;

    LIST_ST_FUNCS(DEFINE_ST_DISPATCH_FIELD);
};


static android::base::LazyInstance<STLibrary> sLib = LAZY_INSTANCE_INIT;

struct ShaderSpecKey {
    GLenum shaderType;
    int esslVersion;
};

static ShShaderSpec sInputSpecForVersion(int esslVersion) {
    switch (esslVersion) {
        case 100:
            return SH_GLES2_SPEC;
        case 300:
            return SH_GLES3_SPEC;
        case 310:
            return SH_GLES3_1_SPEC;
    }
    return SH_GLES3_1_SPEC;
}

static ShShaderOutput sOutputSpecForVersion(bool coreProfileHost, int esslVersion) {
    switch (esslVersion) {
        case 100:
            if (coreProfileHost) {
                return SH_GLSL_330_CORE_OUTPUT;
            } else {
                return SH_GLSL_COMPATIBILITY_OUTPUT;
            }
        case 300:
            if (coreProfileHost) {
                return SH_GLSL_330_CORE_OUTPUT;
            } else {
                return SH_GLSL_150_CORE_OUTPUT;
            }
        case 310:
            return SH_GLSL_430_CORE_OUTPUT;
    }
    return SH_GLSL_430_CORE_OUTPUT;
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

typedef std::map<ShaderSpecKey, ShHandle, ShaderSpecKeyCompare> ShaderCompilerMap;
static ShaderCompilerMap sCompilerMap;

static ShHandle getShaderCompiler(bool coreProfileHost, ShaderSpecKey key) {
    if (sCompilerMap.find(key) == sCompilerMap.end()) {
        sCompilerMap[key] =
            sLib->STConstructCompiler(
                    key.shaderType,
                    sInputSpecForVersion(key.esslVersion),
                    sOutputSpecForVersion(coreProfileHost, key.esslVersion),
                    &kResources);
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
            int maxDualSourceDrawBuffers) {

    sLib->STGenerateResources(&kResources);

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

    sLib.get();

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

    kInitialized = true;
    return true;
}

static void getShaderLinkInfo(int esslVersion,
                              ShHandle compilerHandle,
                              ShaderLinkInfo* linkInfo) {
    linkInfo->esslVersion = esslVersion;

    struct st_name_hashing_map raw_map = sLib->STGetNameHashingMap(compilerHandle);

    for (int i = 0; i < raw_map.count; i++) {
        fprintf(stderr, "%s: usr %s -> compiled %s\n", __func__, raw_map.user_names[i], raw_map.compiled_names[i]);
        linkInfo->nameMap[raw_map.user_names[i]] = raw_map.compiled_names[i];
    }

    int uniformCount;
    int varyingsCount;
    int attributesCount;
    int outputVarsCount;
    int interfaceBlocksCount;

    sLib->STGetUniforms(compilerHandle, &uniformCount, nullptr);
    sLib->STGetVaryings(compilerHandle, &varyingsCount, nullptr);
    sLib->STGetAttributes(compilerHandle, &attributesCount, nullptr);
    sLib->STGetOutputVariables(compilerHandle, &outputVarsCount, nullptr);
    sLib->STGetInterfaceBlocks(compilerHandle, &interfaceBlocksCount, nullptr);

    linkInfo->uniforms.resize(uniformCount);
    linkInfo->varyings.resize(varyingsCount);
    linkInfo->attributes.resize(attributesCount);
    linkInfo->outputVars.resize(outputVarsCount);
    linkInfo->interfaceBlocks.resize(interfaceBlocksCount);

    sLib->STGetUniforms(compilerHandle, &uniformCount, linkInfo->uniforms.data());
    sLib->STGetVaryings(compilerHandle, &varyingsCount, linkInfo->varyings.data());
    sLib->STGetAttributes(compilerHandle, &attributesCount, linkInfo->attributes.data());
    sLib->STGetOutputVariables(compilerHandle, &outputVarsCount, linkInfo->outputVars.data());
    sLib->STGetInterfaceBlocks(compilerHandle, &interfaceBlocksCount, linkInfo->interfaceBlocks.data());

    for (int i = 0; i < uniformCount; i++) {
        linkInfo->nameMap[linkInfo->uniforms[i].name] = linkInfo->uniforms[i].mapped_name;
    }

    for (int i = 0; i < varyingsCount; i++) {
        linkInfo->nameMap[linkInfo->varyings[i].name] = linkInfo->varyings[i].mapped_name;
    }

    for (int i = 0; i < attributesCount; i++) {
        linkInfo->nameMap[linkInfo->attributes[i].name] = linkInfo->attributes[i].mapped_name;
    }

    for (int i = 0; i < outputVarsCount; i++) {
        linkInfo->nameMap[linkInfo->outputVars[i].name] = linkInfo->outputVars[i].mapped_name;
    }
    
    for (const auto& elt : linkInfo->nameMap) {
        linkInfo->nameMapReverse[elt.second] = elt.first;
    }
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

    // Leverage ARB_ES3_1_compatibility for ESSL 310 for now.
    // Use translator after rest of dEQP-GLES31.functional is in a better state.
    if (esslVersion == 310) {
        // Don't try to get obj code just yet.
        // At least on NVIDIA Quadro K2200 Linux (361.xx),
        // ARB_ES3_1_compatibility seems to assume incorrectly
        // that atomic_uint must catch a precision qualifier in ESSL 310.
        std::string origSrc(src);

        outShaderLinkInfo->esslVersion = esslVersion;
        size_t versionStart = origSrc.find("#version");
        size_t versionEnd = origSrc.find("\n", versionStart);
        std::string versionPart = origSrc.substr(versionStart, versionEnd - versionStart + 1);
        std::string src2 =
            versionPart + "precision highp atomic_uint;\n" +
            origSrc.substr(versionEnd + 1, origSrc.size() - (versionEnd + 1));
        *outObjCode = src2;
        return true;
    }

    if (!kInitialized) {
        return false;
    }

    // ANGLE may crash if multiple RenderThreads attempt to compile shaders
    // at the same time.
    android::base::AutoLock autolock(kCompilerLock);

    ShaderSpecKey key;
    key.shaderType = shaderType;
    key.esslVersion = esslVersion;

    ShHandle compilerHandle = getShaderCompiler(hostUsesCoreProfile, key);

    if (!compilerHandle) {
        fprintf(stderr, "%s: no compiler handle for shader type 0x%x, ESSL version %d\n",
                __FUNCTION__,
                shaderType,
                esslVersion);
        return false;
    }

    // Pass in the entire src as 1 string, ask for compiled GLSL object code
    // and information about all compiled variables.
    int res = sLib->STCompile(compilerHandle, &src, 1, SH_OBJECT_CODE | SH_VARIABLES);

    // The compilers return references that may not be valid in the future,
    // and we manually clear them immediately anyway.
    *outInfolog = std::string(sLib->STGetInfoLog(compilerHandle));
    *outObjCode = std::string(sLib->STGetObjectCode(compilerHandle));

    fprintf(stderr, "%s: tr %s out info log  %s obj %s\n", __func__,
            src,
            outInfolog->c_str(),
            outObjCode->c_str());
    if (outShaderLinkInfo) getShaderLinkInfo(esslVersion, compilerHandle, outShaderLinkInfo);

    sLib->STClearResults(compilerHandle);

    return res;
}

}
