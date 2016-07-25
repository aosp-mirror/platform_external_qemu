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
NameSpace::genName(ObjectLocalName p_localName, bool genLocal)
{
    ObjectLocalName localName = p_localName;
    if (genLocal) {
        do {
            localName = ++m_nextName;
        } while(localName == 0 ||
                m_localToGlobalMap.find(localName) !=
                        m_localToGlobalMap.end() );
    }

    unsigned int globalName = m_globalNameSpace->genName(m_type);
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
        if (m_type != TEXTURE) {
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
        if (m_type != TEXTURE) {
            m_globalNameSpace->deleteName(m_type, (*n).second);
        }
        m_globalToLocalMap.erase(n->second);
        (*n).second = p_globalName;
        m_globalToLocalMap.emplace(p_globalName, p_localName);
    }
}

unsigned int
GlobalNameSpace::genName(NamedObjectType p_type)
{
    if ( p_type >= NUM_OBJECT_TYPES ) return 0;
    unsigned int name = 0;

    emugl::Mutex::AutoLock _lock(m_lock);
    switch (p_type) {
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
    case SHADER: //objects in shader namepace are not handled
    default:
        name = 0;
    }
    if (!m_deleteInitialized) {
        m_deleteInitialized = true;
        m_glDelete[VERTEXBUFFER] = GLEScontext::dispatcher().glDeleteBuffers;
        m_glDelete[TEXTURE] = GLEScontext::dispatcher().glDeleteTextures;
        m_glDelete[RENDERBUFFER] =
                GLEScontext::dispatcher().glDeleteRenderbuffersEXT;
        m_glDelete[FRAMEBUFFER] =
                GLEScontext::dispatcher().glDeleteFramebuffersEXT;
    }
    return name;
}

void
GlobalNameSpace::deleteName(NamedObjectType p_type, unsigned int p_name)
{
    if (p_type != SHADER) {
        emugl::Mutex::AutoLock _lock(m_lock);
        m_glDelete[p_type](1, &p_name);
    }
}
