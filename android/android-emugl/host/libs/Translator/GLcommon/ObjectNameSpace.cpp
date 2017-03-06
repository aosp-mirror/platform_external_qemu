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

#include "android/base/containers/Lookup.h"
#include "android/base/files/StreamSerializing.h"
#include "GLcommon/GLEScontext.h"
#include "GLcommon/TranslatorIfaces.h"

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
NameSpace::setGlobalObject(ObjectLocalName p_localName,
                               NamedObjectPtr p_namedObject) {
    NamesMap::iterator n(m_localToGlobalMap.find(p_localName));
    if (n != m_localToGlobalMap.end()) {
        m_globalToLocalMap.erase(n->second->getGlobalName());
        (*n).second = p_namedObject;
    } else {
        m_localToGlobalMap.emplace(p_localName, p_namedObject);
    }
    m_globalToLocalMap.emplace(p_namedObject->getGlobalName(), p_localName);
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

void GlobalNameSpace::preSaveAddEglImage(const EglImage* eglImage) {
    unsigned int globalName = eglImage->globalTexObj->getGlobalName();
    emugl::Mutex::AutoLock lock(m_lock);
    m_textureMap.emplace(globalName,
            SaveableTexturePtr(new SaveableTexture(*eglImage)));
}

void GlobalNameSpace::preSaveAddTex(const TextureData* texture) {
    emugl::Mutex::AutoLock lock(m_lock);
    m_textureMap.emplace(texture->globalName,
            SaveableTexturePtr(new SaveableTexture(*texture)));
}

void GlobalNameSpace::onSave(android::base::Stream* stream,
        std::function<void(SaveableTexture*, android::base::Stream*)> saver) {
    saveCollection(stream, m_textureMap,
            [saver](android::base::Stream* stream,
                const std::pair<const unsigned int, SaveableTexturePtr>& tex) {
                stream->putBe32(tex.first);
                saver(tex.second.get(), stream);
            });
    m_textureMap.clear();
}

void GlobalNameSpace::onLoad(android::base::Stream* stream,
        std::function<SaveableTexture*(android::base::Stream*,
            GlobalNameSpace*)> loader) {
    assert(m_textureMap.size() == 0);
    loadCollection(stream, &m_textureMap, [loader, this](
            android::base::Stream* stream) {
        unsigned int globalName = stream->getBe32();
        SaveableTexture* saveableTexture = loader(stream, this);
        return std::make_pair(globalName, SaveableTexturePtr(saveableTexture));
    });
}

void GlobalNameSpace::postLoad(android::base::Stream* stream) {
    m_textureMap.clear();
}

NamedObjectPtr GlobalNameSpace::getGlobalObjectFromLoad(
        unsigned int oldGlobalName) {
    assert(m_textureMap.count(oldGlobalName));
    return m_textureMap[oldGlobalName]->getGlobalObject();
}

EglImage* GlobalNameSpace::makeEglImageFromLoad(
        unsigned int oldGlobalName) {
    assert(m_textureMap.count(oldGlobalName));
    return m_textureMap[oldGlobalName]->makeEglImage();
}

