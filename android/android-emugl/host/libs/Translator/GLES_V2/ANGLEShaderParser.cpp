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
#include "android/base/synchronization/MessageChannel.h"
#include "android/base/threads/Thread.h"

#include <map>
#include <string>

#define SH_GLES31_SPEC ((ShShaderSpec)0x8B88)
#define GL_COMPUTE_SHADER 0x91B9

namespace ANGLEShaderParser {

using android::base::Thread;

android::base::Lock kCompilerLock;
ShBuiltInResources kResources;
bool kInitialized = false;

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
            return SH_GLES31_SPEC;
    }
    return SH_GLES31_SPEC;
}

static ShShaderOutput sOutputSpecForVersion(int esslVersion) {
    switch (esslVersion) {
        case 100:
        case 300:
            return SH_GLSL_330_CORE_OUTPUT;
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

static ShHandle getShaderCompiler(ShaderSpecKey key) {
    if (sCompilerMap.find(key) == sCompilerMap.end()) {
        sCompilerMap[key] =
            ShConstructCompiler(
                    key.shaderType,
                    sInputSpecForVersion(key.esslVersion),
                    sOutputSpecForVersion(key.esslVersion),
                    &kResources);
    }
    return sCompilerMap[key];
}

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

    kResources.OES_standard_derivatives = 1;
    kResources.OES_EGL_image_external = 0;
    kResources.EXT_gpu_shader5 = 1;
}

static void getShaderLinkInfo(ShHandle compilerHandle,
                              ShaderLinkInfo* linkInfo) {
    auto uniforms = ShGetUniforms(compilerHandle);
    auto varyings = ShGetVaryings(compilerHandle);
    auto attributes = ShGetAttributes(compilerHandle);
    auto outputVars = ShGetOutputVariables(compilerHandle);
    auto interfaceBlocks = ShGetInterfaceBlocks(compilerHandle);

    if (uniforms) linkInfo->uniforms = *uniforms;
    if (varyings) linkInfo->varyings = *varyings;
    if (attributes) linkInfo->attributes = *attributes;
    if (outputVars) linkInfo->outputVars = *outputVars;
    if (interfaceBlocks) linkInfo->interfaceBlocks = *interfaceBlocks;
}

class ANGLEThread : public Thread {
public:
    enum Opcode {
        GLOBAL_INIT = 0,
        TRANSLATE = 1,
        EXIT = 2,
    };

    struct Cmd {
        Opcode opCode = GLOBAL_INIT;

        int attribs;
        int uniformVectors;
        int varyingVectors;
        int vertexTextureImageUnits;
        int combinedTexImageUnits;
        int textureImageUnits;
        int fragmentUniformVectors;
        int drawBuffers;
        int fragmentPrecisionHigh;
        int vertexOutputComponents;
        int fragmentInputComponents;
        int minProgramTexelOffset;
        int maxProgramTexelOffset;
        int maxDualSourceDrawBuffers;

        int esslVersion;
        const char* src;
        GLenum shaderType;
        std::string* outInfolog;
        std::string* outObjCode;
        ShaderLinkInfo* outShaderLinkInfo;
    };

    ANGLEThread() { this->start(); }
    void sendAndWaitForResult(const Cmd& cmd) {
        mInput.send(cmd);
        int res = -1;
        mOutput.receive(&res);
        (void)res;
        return;
    }

private:
    static const size_t kCmdsMax = 16;
    android::base::MessageChannel<Cmd, kCmdsMax> mInput;
    android::base::MessageChannel<int, kCmdsMax> mOutput;
    Cmd mCurrentCmd;
    
    virtual intptr_t main() override final {
        bool exiting = false;
        while (!exiting) {
            mInput.receive(&mCurrentCmd);
            doCmd(mCurrentCmd);
            mOutput.send(0);
            if (mCurrentCmd.opCode == EXIT) exiting = true;
        }
        return 0;
    }

    void doCmd(const Cmd& cmd) {
        switch (cmd.opCode) {
            case GLOBAL_INIT:
                if (!kInitialized && !ShInitialize()) {
                    fprintf(stderr, "Global ANGLE shader compiler initialzation failed.\n");
                }

                initializeResources(
                        cmd.attribs,
                        cmd.uniformVectors,
                        cmd.varyingVectors,
                        cmd.vertexTextureImageUnits,
                        cmd.combinedTexImageUnits,
                        cmd.textureImageUnits,
                        cmd.fragmentUniformVectors,
                        cmd.drawBuffers,
                        cmd.fragmentPrecisionHigh,
                        cmd.vertexOutputComponents,
                        cmd.fragmentInputComponents,
                        cmd.minProgramTexelOffset,
                        cmd.maxProgramTexelOffset,
                        cmd.maxDualSourceDrawBuffers);

                kInitialized = true;
                break;
            case TRANSLATE: {
                // Leverage ARB_ES3_1_compatibility for ESSL 310 for now.
                // Use translator after rest of dEQP-GLES31.functional is in a better state.
                if (cmd.esslVersion == 310) {
                    // At least on NVIDIA Quadro K2200 Linux (361.xx),
                    // ARB_ES3_1_compatibility seems to assume incorrectly
                    // that atomic_uint must catch a precision qualifier in ESSL 310.
                    std::string origSrc(cmd.src);
                    size_t versionStart = origSrc.find("#version");
                    size_t versionEnd = origSrc.find("\n", versionStart);
                    std::string versionPart = origSrc.substr(versionStart, versionEnd - versionStart + 1);
                    std::string src2 =
                        versionPart + "precision highp atomic_uint;\n" +
                        origSrc.substr(versionEnd + 1, origSrc.size() - (versionEnd + 1));
                    *cmd.outObjCode = src2;
                }

                if (!kInitialized) {
                }

                // ANGLE may crash if multiple RenderThreads attempt to compile shaders
                // at the same time.
                android::base::AutoLock autolock(kCompilerLock);

                ShaderSpecKey key;
                key.shaderType = cmd.shaderType;
                key.esslVersion = cmd.esslVersion;

                ShHandle compilerHandle = getShaderCompiler(key);

                if (!compilerHandle) {
                    fprintf(stderr, "%s: no compiler handle for shader type 0x%x, ESSL version %d\n",
                            __FUNCTION__,
                            cmd.shaderType,
                            cmd.esslVersion);
                }

                // Pass in the entire src as 1 string, ask for compiled GLSL object code
                // and information about all compiled variables.
                int res = ShCompile(compilerHandle, &cmd.src, 1, SH_OBJECT_CODE | SH_VARIABLES);

                // The compilers return references that may not be valid in the future,
                // and we manually clear them immediately anyway.
                *cmd.outInfolog = std::string(ShGetInfoLog(compilerHandle));
                *cmd.outObjCode = std::string(ShGetObjectCode(compilerHandle));

                fprintf(stderr, "%s: out obj code %s\n", __func__, cmd.outObjCode->c_str());
                if (cmd.outShaderLinkInfo) getShaderLinkInfo(compilerHandle, cmd.outShaderLinkInfo);
                ShClearResults(compilerHandle);
                break; }
            case EXIT:
                break;
        }
    }
};

static ANGLEThread sThread;

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

    ANGLEThread::Cmd cmd;
    cmd.opCode = ANGLEThread::GLOBAL_INIT;
    cmd.attribs = attribs;
    cmd.uniformVectors = uniformVectors;
    cmd.varyingVectors = varyingVectors;
    cmd.vertexTextureImageUnits = vertexTextureImageUnits;
    cmd.combinedTexImageUnits = combinedTexImageUnits;
    cmd.textureImageUnits = textureImageUnits;
    cmd.fragmentUniformVectors = fragmentUniformVectors;
    cmd.drawBuffers = drawBuffers;
    cmd.fragmentPrecisionHigh = fragmentPrecisionHigh;
    cmd.vertexOutputComponents = vertexOutputComponents;
    cmd.fragmentInputComponents = fragmentInputComponents;
    cmd.minProgramTexelOffset = minProgramTexelOffset;
    cmd.maxProgramTexelOffset = maxProgramTexelOffset;
    cmd.maxDualSourceDrawBuffers = maxDualSourceDrawBuffers;
    sThread.sendAndWaitForResult(cmd);
    kInitialized = true;
    return true;
}

bool translate(int esslVersion,
               const char* src,
               GLenum shaderType,
               std::string* outInfolog,
               std::string* outObjCode,
               ShaderLinkInfo* outShaderLinkInfo) {
    ANGLEThread::Cmd cmd;
    cmd.opCode = ANGLEThread::TRANSLATE;
    cmd.esslVersion = esslVersion;
    cmd.src = src;
    cmd.shaderType = shaderType;
    cmd.outInfolog = outInfolog;
    cmd.outObjCode = outObjCode;
    cmd.outShaderLinkInfo = outShaderLinkInfo;
    sThread.sendAndWaitForResult(cmd);
    return true;
}

}
