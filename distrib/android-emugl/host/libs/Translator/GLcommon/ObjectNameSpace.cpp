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

#include "GLcommon/ObjectNameSpace.h"

#include "GLcommon/GLEScontext.h"

#include <assert.h>

NameSpace::NameSpace(NamedObjectType p_type,
                     GlobalNameSpace *globalNameSpace) :
    m_type(p_type),
    m_globalNameSpace(globalNameSpace) {}

NameSpace::~NameSpace()
{
    for (NamesMap::iterator n = m_localToGlobalMap.begin();
         n != m_localToGlobalMap.end();
         ++n) {
        m_globalNameSpace->deleteName(m_type, (*n).second);
    }
}

ObjectLocalName
NameSpace::genName(GenNameInfo genNameInfo, ObjectLocalName p_localName, bool genLocal)
{
    assert(m_type == genNameInfo.m_type);
    ObjectLocalName localName = p_localName;
    if (genLocal) {
        do {
            localName = ++m_nextName;
        } while(localName == 0 ||
                m_localToGlobalMap.find(localName) !=
                        m_localToGlobalMap.end() );
    }

    unsigned int globalName = m_globalNameSpace->genName(genNameInfo);
    m_localToGlobalMap[localName] = globalName;
    m_globalToLocalMap[globalName] = localName;

    return localName;
}


unsigned int
NameSpace::getGlobalName(ObjectLocalName p_localName)
{
    NamesMap::iterator n( m_localToGlobalMap.find(p_localName) );
    if (n != m_localToGlobalMap.end()) {
        // object found - return its global name map
        return (*n).second;
    }

    // object does not exist;
    return 0;
}

ObjectLocalName
NameSpace::getLocalName(unsigned int p_globalName)
{
    const auto it = m_globalToLocalMap.find(p_globalName);
    if (it != m_globalToLocalMap.end()) {
        return it->second;
    }

    return 0;
}

void
NameSpace::deleteName(ObjectLocalName p_localName)
{
    NamesMap::iterator n( m_localToGlobalMap.find(p_localName) );
    if (n != m_localToGlobalMap.end()) {
        if (m_type != NamedObjectType::TEXTURE) {
            m_globalNameSpace->deleteName(m_type, (*n).second);
        }
        m_globalToLocalMap.erase(n->second);
        m_localToGlobalMap.erase(n);
    }
}

bool
NameSpace::isObject(ObjectLocalName p_localName)
{
    return (m_localToGlobalMap.find(p_localName) != m_localToGlobalMap.end() );
}

void
NameSpace::replaceGlobalName(ObjectLocalName p_localName, unsigned int p_globalName)
{
    NamesMap::iterator n( m_localToGlobalMap.find(p_localName) );
    if (n != m_localToGlobalMap.end()) {
        if (m_type != NamedObjectType::TEXTURE) {
            m_globalNameSpace->deleteName(m_type, (*n).second);
        }
        m_globalToLocalMap.erase(n->second);
        (*n).second = p_globalName;
        m_globalToLocalMap.emplace(p_globalName, p_localName);
    }
}

unsigned int
GlobalNameSpace::genName(GenNameInfo genNameInfo)
{
    if (genNameInfo.m_type >= NamedObjectType::NUM_OBJECT_TYPES)
        return 0;
    unsigned int name = 0;

    emugl::Mutex::AutoLock _lock(m_lock);
    switch (genNameInfo.m_type) {
        case NamedObjectType::VERTEXBUFFER:
            GLEScontext::dispatcher().glGenBuffers(1, &name);
            break;
        case NamedObjectType::TEXTURE:
            GLEScontext::dispatcher().glGenTextures(1, &name);
            break;
        case NamedObjectType::RENDERBUFFER:
            GLEScontext::dispatcher().glGenRenderbuffersEXT(1, &name);
            break;
        case NamedObjectType::FRAMEBUFFER:
            GLEScontext::dispatcher().glGenFramebuffersEXT(1, &name);
            break;
        case NamedObjectType::SHADER:
            name = GLEScontext::dispatcher().glCreateShader(
                    genNameInfo.m_shaderType);
            break;
        case NamedObjectType::PROGRAM:
            name = GLEScontext::dispatcher().glCreateProgram();
            break;
        default:
            name = 0;
    }
    if (!m_deleteInitialized) {
        m_deleteInitialized = true;
        m_glDelete[static_cast<int>(NamedObjectType::VERTEXBUFFER)] =
                [](GLuint lname) {
                    GLEScontext::dispatcher().glDeleteBuffers(1, &lname);
                };
        m_glDelete[static_cast<int>(NamedObjectType::TEXTURE)] =
                [](GLuint lname) {
                    GLEScontext::dispatcher().glDeleteTextures(1, &lname);
                };
        m_glDelete[static_cast<int>(NamedObjectType::RENDERBUFFER)] = [](
                GLuint lname) {
            GLEScontext::dispatcher().glDeleteRenderbuffersEXT(1, &lname);
        };
        m_glDelete[static_cast<int>(NamedObjectType::FRAMEBUFFER)] = [](
                GLuint lname) {
            GLEScontext::dispatcher().glDeleteFramebuffersEXT(1, &lname);
        };
        m_glDelete[static_cast<int>(NamedObjectType::PROGRAM)] =
                GLEScontext::dispatcher().glDeleteProgram;
        m_glDelete[static_cast<int>(NamedObjectType::SHADER)] =
                GLEScontext::dispatcher().glDeleteShader;
    }
    return name;
}

void
GlobalNameSpace::deleteName(NamedObjectType p_type, unsigned int p_name)
{
    if (p_type < NamedObjectType::NUM_OBJECT_TYPES && p_name != 0) {
        m_glDelete[static_cast<int>(p_type)](p_name);
    }
}
