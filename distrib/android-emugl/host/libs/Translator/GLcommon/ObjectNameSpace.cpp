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

    auto it = m_localToGlobalMap.emplace(localName,
                                         NamedObjectPtr(
                                            new NamedObject(genNameInfo,
                                                    m_globalNameSpace))).first;
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
