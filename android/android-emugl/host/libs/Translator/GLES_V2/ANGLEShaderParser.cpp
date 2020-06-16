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

#define GL_COMPUTE_SHADER 0x91B9

namespace ANGLEShaderParser {

ST_BuiltInResources kResources;
bool kInitialized = false;

class LazyLoadedSTDispatch {
public:
    LazyLoadedSTDispatch() {
        memset(&mDispatch, 0, sizeof(STDispatch));

#ifdef __APPLE__
        const char kLibName[] = "libshadertranslator.dylib";
#elif defined(_WIN32)
        const char kLibName[] = "libshadertranslator.dll";
#else
        const char kLibName[] = "libshadertranslator.so";
#endif
        char error[256];
        mLib = emugl::SharedLibrary::open(kLibName, error, sizeof(error));
        if (!mLib) {
            fprintf(stderr, "%s: Could not open shader translator library %s [%s]\n",
                __func__, kLibName, error);
            return;
        }

        mDispatch.initialize =
            (STInitialize_t)mLib->findSymbol("STInitialize");
        mDispatch.finalize =
            (STFinalize_t)mLib->findSymbol("STFinalize");
        mDispatch.generateResources =
            (STGenerateResources_t)mLib->findSymbol("STGenerateResources");
        mDispatch.compileAndResolve =
            (STCompileAndResolve_t)mLib->findSymbol("STCompileAndResolve");
        mDispatch.freeShaderResolveState =
            (STFreeShaderResolveState_t)mLib->findSymbol("STFreeShaderResolveState");
        mDispatch.copyVariable =
            (STCopyVariable_t)mLib->findSymbol("STCopyVariable");
        mDispatch.copyInterfaceBlock =
            (STCopyInterfaceBlock_t)mLib->findSymbol("STCopyInterfaceBlock");
        mDispatch.destroyVariable =
            (STDestroyVariable_t)mLib->findSymbol("STDestroyVariable");
        mDispatch.destroyInterfaceBlock =
            (STDestroyInterfaceBlock_t)mLib->findSymbol("STDestroyInterfaceBlock");

        mValid = dispatchValid();

        if (!mValid) {
            fprintf(stderr, "%s: error, shader translator dispatch not valid\n", __func__);
        }
    }

    STDispatch* getDispatch() {
        if (!mValid) return nullptr;
        return &mDispatch;
    }

private:
    bool dispatchValid() {
        return (nullptr != mDispatch.initialize) &&
               (nullptr != mDispatch.finalize) &&
               (nullptr != mDispatch.generateResources) &&
               (nullptr != mDispatch.compileAndResolve) &&
               (nullptr != mDispatch.copyVariable) &&
               (nullptr != mDispatch.copyInterfaceBlock) &&
               (nullptr != mDispatch.destroyVariable) &&
               (nullptr != mDispatch.destroyInterfaceBlock);
    }

    emugl::SharedLibrary* mLib = nullptr;
    bool mValid = false;
    STDispatch mDispatch;
};

static android::base::LazyInstance<LazyLoadedSTDispatch> sLazyLoadedSTDispatch =
    LAZY_INSTANCE_INIT;

STDispatch* getSTDispatch() {
    return sLazyLoadedSTDispatch->getDispatch();
}

ShaderLinkInfo::ShaderLinkInfo() = default;
ShaderLinkInfo::ShaderLinkInfo(const ShaderLinkInfo& other) {
    clear();
    copyFromOther(other);
}

ShaderLinkInfo& ShaderLinkInfo::operator=(const ShaderLinkInfo& other) {
    if (this != &other) {
        ShaderLinkInfo tmp(other);
        *this = std::move(tmp);
    }
    return *this;
}

ShaderLinkInfo::ShaderLinkInfo(ShaderLinkInfo&& other) {
    *this = std::move(other);
}

ShaderLinkInfo& ShaderLinkInfo::operator=(ShaderLinkInfo&& other) {
    esslVersion = other.esslVersion;
    uniforms = std::move(other.uniforms);
    varyings = std::move(other.varyings);
    attributes = std::move(other.attributes);
    outputVars = std::move(other.outputVars);
    nameMap = std::move(other.nameMap);
    nameMapReverse = std::move(other.nameMapReverse);

    return *this;
}

ShaderLinkInfo::~ShaderLinkInfo() {
    clear();
}

void ShaderLinkInfo::copyFromOther(const ShaderLinkInfo& other) {
    auto dispatch = getSTDispatch();
    esslVersion = other.esslVersion;
    for (const auto& var: other.uniforms) { uniforms.push_back(dispatch->copyVariable(&var)); }
    for (const auto& var: other.varyings) { varyings.push_back(dispatch->copyVariable(&var)); }
    for (const auto& var: other.attributes) { attributes.push_back(dispatch->copyVariable(&var)); }
    for (const auto& var: other.outputVars) { outputVars.push_back(dispatch->copyVariable(&var)); }
    for (const auto& var: other.interfaceBlocks) { interfaceBlocks.push_back(dispatch->copyInterfaceBlock(&var)); }
    nameMap = other.nameMap;
    nameMapReverse = other.nameMapReverse;
}

void ShaderLinkInfo::clear() {
    auto dispatch = getSTDispatch();
    for (auto& var: uniforms) { dispatch->destroyVariable(&var); }
    for (auto& var: varyings) { dispatch->destroyVariable(&var); }
    for (auto& var: attributes) { dispatch->destroyVariable(&var); }
    for (auto& var: outputVars) { dispatch->destroyVariable(&var); }
    for (auto& var: interfaceBlocks) { dispatch->destroyInterfaceBlock(&var); }
    uniforms.clear();
    varyings.clear();
    attributes.clear();
    outputVars.clear();
    interfaceBlocks.clear();
    nameMap.clear();
    nameMapReverse.clear();
}

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
static android::base::LazyInstance<ShaderCompilerMap> sCompilerMap = LAZY_INSTANCE_INIT;

static ST_Handle getShaderCompiler(bool coreProfileHost, ShaderSpecKey key) {
    auto it = sCompilerMap->find(key);
    if (it == sCompilerMap->end()) return (ST_Handle)nullptr;
    return it->second;
}

android::base::Lock kCompilerLock;

void initializeResources(
    BuiltinResourcesEditCallback callback) {

    getSTDispatch()->generateResources(&kResources);

    callback(kResources);
}

bool globalInitialize(
    bool isGles2Gles,
    BuiltinResourcesEditCallback editCallback) {

    if (!isGles2Gles) {
        getSTDispatch()->initialize();
    }

    initializeResources(editCallback);

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
    linkInfo->uniforms.clear();
    linkInfo->varyings.clear();
    linkInfo->attributes.clear();
    linkInfo->outputVars.clear();
    linkInfo->interfaceBlocks.clear();

    for (uint32_t i = 0; i < compileResult->nameHashingMap->entryCount; ++i) {
        linkInfo->nameMap[compileResult->nameHashingMap->ppUserNames[i]] =
            compileResult->nameHashingMap->ppCompiledNames[i];
    }

    for (const auto& elt : linkInfo->nameMap) {
        linkInfo->nameMapReverse[elt.second] = elt.first;
    }

    auto st = getSTDispatch();
    auto stCopyVar = st->copyVariable;
    auto stCopyIb = st->copyInterfaceBlock;

    linkInfo->uniforms = convertArrayToVecWithCopy(
        compileResult->uniformsCount,
        compileResult->pUniforms,
        stCopyVar);

    std::vector<ST_ShaderVariable> inputVaryings =
        convertArrayToVecWithCopy(
            compileResult->inputVaryingsCount, compileResult->pInputVaryings,
            stCopyVar);
    std::vector<ST_ShaderVariable> outputVaryings =
        convertArrayToVecWithCopy(
            compileResult->outputVaryingsCount, compileResult->pOutputVaryings,
            stCopyVar);

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
            stCopyVar);

    linkInfo->outputVars =
        convertArrayToVecWithCopy(
            compileResult->activeOutputVariablesCount,
            compileResult->pActiveOutputVariables,
            stCopyVar);

    linkInfo->interfaceBlocks =
        convertArrayToVecWithCopy(
            compileResult->uniformBlocksCount,
            compileResult->pUniformBlocks,
            stCopyIb);
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
        size_t extensionStart = origSrc.rfind("#extension");
        size_t extensionEnd = origSrc.find("\n", extensionStart);
        if (extensionStart == std::string::npos) {
            std::string versionPart = origSrc.substr(versionStart, versionEnd - versionStart + 1);
            std::string src2 =
                versionPart + "precision highp atomic_uint;\n" +
                origSrc.substr(versionEnd + 1, origSrc.size() - (versionEnd + 1));
            *outObjCode = src2;
        } else {
            std::string uptoExtensionPart = origSrc.substr(0, extensionEnd + 1);
            std::string src2 =
                uptoExtensionPart + "precision highp atomic_uint;\n" +
                origSrc.substr(extensionEnd + 1, origSrc.size() - (extensionEnd + 1));
            *outObjCode = src2;
        }
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

    auto st = getSTDispatch();

    st->compileAndResolve(&ci, &res);

    sCompilerMap->emplace(key, res->outputHandle);
    *outInfolog = std::string(res->infoLog);
    *outObjCode = std::string(res->translatedSource);

    if (outShaderLinkInfo) getShaderLinkInfo(esslVersion, res, outShaderLinkInfo);

    bool ret = res->compileStatus == 1;

    st->freeShaderResolveState(res);
    return ret;
}

} // namespace ANGLEShaderParser
