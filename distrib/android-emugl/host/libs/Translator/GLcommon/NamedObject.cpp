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

#include "GLcommon/NamedObject.h"

#include "emugl/common/mutex.h"
#include "GLcommon/GLEScontext.h"
#include "GLcommon/ObjectNameSpace.h"

NamedObject::NamedObject(GenNameInfo genNameInfo,
                         GlobalNameSpace *globalNameSpace) {
    // We need a global mutex from globalNameSpace
    // And we can't change the mutex to a static variable because it would be
    // passed around different shared libraries.
    m_globalNameSpace = globalNameSpace;
    emugl::Mutex::AutoLock _lock(m_globalNameSpace->m_lock);

    switch (genNameInfo.m_type) {
    case NamedObjectType::VERTEXBUFFER:
        GLEScontext::dispatcher().glGenBuffers(1,&m_globalName);
        m_glDelete = [](GLuint lname)
                { GLEScontext::dispatcher().glDeleteBuffers(1, &lname);};
        break;
    case NamedObjectType::TEXTURE:
        GLEScontext::dispatcher().glGenTextures(1,&m_globalName);
        m_glDelete = [](GLuint lname)
                { GLEScontext::dispatcher().glDeleteTextures(1, &lname);};
        break;
    case NamedObjectType::RENDERBUFFER:
        GLEScontext::dispatcher().glGenRenderbuffersEXT(1, &m_globalName);
        m_glDelete = [](GLuint lname)
                { GLEScontext::dispatcher().glDeleteRenderbuffersEXT(1, &lname);};
        break;
    case NamedObjectType::FRAMEBUFFER:
        GLEScontext::dispatcher().glGenFramebuffersEXT(1,&m_globalName);
        m_glDelete = [](GLuint lname)
                { GLEScontext::dispatcher().glDeleteFramebuffersEXT(1, &lname);};
        break;
    case NamedObjectType::SHADER:
        m_globalName = GLEScontext::dispatcher().glCreateShader(
                                            genNameInfo.m_shaderType);
        m_glDelete = [](GLuint lname)
                { GLEScontext::dispatcher().glDeleteShader(lname);};
        break;
    case NamedObjectType::PROGRAM:
        m_globalName = GLEScontext::dispatcher().glCreateProgram();
        m_glDelete = [](GLuint lname)
                { GLEScontext::dispatcher().glDeleteProgram(lname);};
        break;
    default:
        m_globalName = 0;
    }
}

NamedObject::~NamedObject() {
    emugl::Mutex::AutoLock _lock(m_globalNameSpace->m_lock);
    m_glDelete(m_globalName);
}

