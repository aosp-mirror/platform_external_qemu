/*
* Copyright (C) 2016 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#pragma once

#include <assert.h>
#include <GLES/gl.h>

enum class NamedObjectType {
    VERTEXBUFFER,
    TEXTURE,
    RENDERBUFFER,
    FRAMEBUFFER,
    SHADER_OR_PROGRAM,
    NUM_OBJECT_TYPES  // Must be last
};

enum class ShaderProgramType {
    PROGRAM,
    VERTEX_SHADER,
    FRAGMENT_SHADER
};

//
// Class GenNameInfo - this class contains the type of an GL object to be
//                     generated. It also contains shader type when generating
//                     shader object.
//                     It is used by GlobalNameSpace::genName.
//                     When generating an object that is not shader or program,
//                     a NamedObjectType parameter should be passed to the
//                     constructor. When generating shader / program object, the
//                     ShaderProgramType should be passed to the constructor.
// Example:
//          GlobalNameSpace globalNameSpace;
//          GenNameInfo genTexture(NamedObjectType::TEXTURE);
//          unsigned int texID = globalNameSpace.genName(genTexture);
//          GenNameInfo genShader(ShaderProgramType::FRAGMENT_SHADER);
//          unsigned int shaderID = globalNameSpace.genName(genShader);

struct GenNameInfo {
    NamedObjectType m_type = (NamedObjectType)0;
    // only used for NamedObjectType::SHADER_OR_PROGRAM
    ShaderProgramType m_shaderProgramType = (ShaderProgramType)0;
    GenNameInfo() = delete;
    // constructor for generating non-shader object
    explicit GenNameInfo(NamedObjectType type) : m_type(type) {
        assert(type != NamedObjectType::SHADER_OR_PROGRAM);
    }
    // constructor for generating shader object
    explicit GenNameInfo(ShaderProgramType shaderProgramType) :
        m_type(NamedObjectType::SHADER_OR_PROGRAM),
        m_shaderProgramType(shaderProgramType) {}
};

typedef unsigned long long ObjectLocalName;

