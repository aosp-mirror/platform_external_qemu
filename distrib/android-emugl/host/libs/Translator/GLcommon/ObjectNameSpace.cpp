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

GenNameInfo::GenNameInfo(const GenNameInfo& genNameInfo)
                        : m_type(genNameInfo.m_type)
                        , m_extraInfo(genNameInfo.m_extraInfo) {}

GenNameInfo::GenNameInfo(NamedObjectType type) : m_type(type) {
    assert(type != SHADER);
}

GenNameInfo::GenNameInfo(NamedObjectType type, GLuint shaderType)
                        : m_type(type) {
    assert(type == SHADER);
    m_extraInfo.shaderType = shaderType;
}

NameSpace::NameSpace(NamedObjectType p_type,
                     GlobalNameSpace *globalNameSpace) :
    m_type(p_type),
    m_globalNameSpace(globalNameSpace) {}

NameSpace::~NameSpace()
{
}

ObjectLocalName
NameSpace::genName(const GenNameInfo& genNameInfo, ObjectLocalName p_localName, bool genLocal)
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

    auto it = m_localToGlobalMap.emplace(p_localName,
                            new NamedObject(m_type,
                                            genNameInfo.m_extraInfo.shaderType,
                                            m_globalNameSpace)).first;
    unsigned int globalName = it->second->getGlobalName();
    m_globalToLocalMap[globalName] = localName;

    return localName;
}


unsigned int
NameSpace::getGlobalName(ObjectLocalName p_localName)
{
    NamesMap::iterator n( m_localToGlobalMap.find(p_localName) );
    if (n != m_localToGlobalMap.end()) {
        // object found - return its global name map
        return (*n).second->getGlobalName();
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

NamedObjectPtr NameSpace::getNamedObject(ObjectLocalName p_localName) {
    auto it = m_localToGlobalMap.find(p_localName);
    if (it != m_localToGlobalMap.end()) {
        return it->second;
    }

    return nullptr;
}

void
NameSpace::deleteName(ObjectLocalName p_localName)
{
    NamesMap::iterator n( m_localToGlobalMap.find(p_localName) );
    if (n != m_localToGlobalMap.end()) {
        m_globalToLocalMap.erase(n->second->getGlobalName());
        m_localToGlobalMap.erase(n);
    }
}

bool
NameSpace::isObject(ObjectLocalName p_localName)
{
    return (m_localToGlobalMap.find(p_localName) != m_localToGlobalMap.end() );
}

void
NameSpace::replaceGlobalObject(ObjectLocalName p_localName,
                               NamedObjectPtr p_namedObject)
{
    NamesMap::iterator n( m_localToGlobalMap.find(p_localName) );
    if (n != m_localToGlobalMap.end()) {
        m_globalToLocalMap.erase(n->second->getGlobalName());
        (*n).second = p_namedObject;
        m_globalToLocalMap.emplace(p_namedObject->getGlobalName(), p_localName);
    }
}

unsigned int
GlobalNameSpace::genName(const GenNameInfo& genNameInfo)
{
    if ( genNameInfo.m_type >= NUM_OBJECT_TYPES ) return 0;
    unsigned int name = 0;

    emugl::Mutex::AutoLock _lock(m_lock);
    switch (genNameInfo.m_type) {
    case VERTEXBUFFER:
        GLEScontext::dispatcher().glGenBuffers(1,&name);
        break;
    case TEXTURE:
        GLEScontext::dispatcher().glGenTextures(1,&name);
        break;
    case RENDERBUFFER:
        GLEScontext::dispatcher().glGenRenderbuffersEXT(1,&name);
        break;
    case FRAMEBUFFER:
        GLEScontext::dispatcher().glGenFramebuffersEXT(1,&name);
        break;
    case SHADER:
        name = GLEScontext::dispatcher().glCreateShader(
                                            genNameInfo.m_extraInfo.shaderType);
        break;
    case PROGRAM:
        name = GLEScontext::dispatcher().glCreateProgram();
        break;
    default:
        name = 0;
    }
    if (!m_deleteInitialized) {
        m_deleteInitialized = true;
        m_glRelease[VERTEXBUFFER] = GLEScontext::dispatcher().glDeleteBuffers;
        m_glRelease[TEXTURE] = GLEScontext::dispatcher().glDeleteTextures;
        m_glRelease[RENDERBUFFER] =
                GLEScontext::dispatcher().glDeleteRenderbuffersEXT;
        m_glRelease[FRAMEBUFFER] =
                GLEScontext::dispatcher().glDeleteFramebuffersEXT;
        m_glDelete[PROGRAM] =
                GLEScontext::dispatcher().glDeleteProgram;
        m_glDelete[SHADER] =
                GLEScontext::dispatcher().glDeleteShader;
    }
    return name;
}

void
GlobalNameSpace::deleteName(NamedObjectType p_type, unsigned int p_name)
{
    emugl::Mutex::AutoLock _lock(m_lock);
    switch (p_type) {
        case SHADER:
        case PROGRAM:
            m_glDelete[p_type](p_name);
            break;
        default:
            m_glRelease[p_type](1, &p_name);
            break;
    }
}
