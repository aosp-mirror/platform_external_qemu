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
    PROGRAM,
    SHADER,
    NUM_OBJECT_TYPES  // Must be last
};

//
// Class GenNameInfo - this class contains the type of an GL object to be
//                     generated. It also contains shader type when generating
//                     shader object.
//                     It is used by GlobalNameSpace::genName.
//                     When generating non-shader object, a NamedObjectType
//                     parameter should be passed to the constructor. When
//                     generating shader object, the shader type should be
//                     passed to the constructor.
// Example:
//          GlobalNameSpace globalNameSpace;
//          GenNameInfo genTexture(NamedObjectType::TEXTURE);
//          unsigned int texID = globalNameSpace.genName(genTexture);
//          GenNameInfo genShader(GL_FRAGMENT_SHADER);
//          unsigned int shaderID = globalNameSpace.genName(genShader);

struct GenNameInfo {
    NamedObjectType m_type = (NamedObjectType)0;
    GLenum m_shaderType = 0;  // only used for NamedObjectType::SHADER
    GenNameInfo() = delete;
    // constructor for generating non-shader object
    explicit GenNameInfo(NamedObjectType type) : m_type(type) {
        assert(type != NamedObjectType::SHADER);
    }
    // constructor for generating shader object
    explicit GenNameInfo(GLenum shaderType)
        : m_type(NamedObjectType::SHADER), m_shaderType(shaderType) {}
};

typedef unsigned long long ObjectLocalName;

