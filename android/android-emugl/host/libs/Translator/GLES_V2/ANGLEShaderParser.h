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

#pragma once

#include <map>
#include <string>
#include <vector>

#include <GLES2/gl2.h>

#include "/usr/local/google/home/lfy/emu/external/angle/src/libShaderTranslator/ShaderTranslator.h"

namespace ANGLEShaderParser {

// Convenient to query those
extern ST_BuiltInResources kResources;

// For performing link-time validation of shader programs.
struct ShaderLinkInfo {
    int esslVersion;
    std::vector<ST_ShaderVariable> uniforms;
    std::vector<ST_ShaderVariable> varyings;
    std::vector<ST_ShaderVariable> attributes;
    std::vector<ST_ShaderVariable> outputVars;
    std::vector<ST_InterfaceBlock> interfaceBlocks;
    std::map<std::string, std::string> nameMap;
    std::map<std::string, std::string> nameMapReverse;
};

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
        bool shaderFramebufferFetch);

bool translate(bool hostUsesCoreProfile,
               const char* src, GLenum shaderType,
               std::string* outInfolog, std::string* outObjCode,
               ShaderLinkInfo* outShaderLinkInfo);
}
