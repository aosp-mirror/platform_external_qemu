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
#include <GLcommon/objectNameManager.h>
#include <GLcommon/ObjectNameSpace.h>
#include <GLcommon/GLEScontext.h>

#include "android/base/containers/Lookup.h"
#include "android/base/memory/LazyInstance.h"

#include <array>
#include <utility>

static constexpr int toIndex(NamedObjectType type) {
    return static_cast<int>(type);
}

using ObjectDataMap =
    std::array<std::unordered_map<ObjectLocalName, ObjectDataPtr>,
               toIndex(NamedObjectType::NUM_OBJECT_TYPES)>;

ShareGroup::ShareGroup(GlobalNameSpace *globalNameSpace,
                       uint64_t sharedGroupID) :
                       m_sharedGroupID(sharedGroupID) {
    for (int i = 0; i < toIndex(NamedObjectType::NUM_OBJECT_TYPES);
         i++) {
        m_nameSpace[i] =
                new NameSpace(static_cast<NamedObjectType>(i), globalNameSpace);
    }
}

void ShareGroup::lockObjectData() {
    while (m_objectsDataLock.test_and_set(std::memory_order_acquire)) {
        ;
    }
}

void ShareGroup::unlockObjectData() {
    m_objectsDataLock.clear(std::memory_order_release);
}

struct ShareGroup::ObjectDataAutoLock {
    ObjectDataAutoLock(ShareGroup* self) : self(self) {
        self->lockObjectData();
    }
    ~ObjectDataAutoLock() {
        self->unlockObjectData();
    }

    ShareGroup* self;
};


ShareGroup::~ShareGroup()
{
    {
        emugl::Mutex::AutoLock lock(m_namespaceLock);
        for (auto n : m_nameSpace) {
            delete n;
        }
    }

    {
        ObjectDataAutoLock lock(this);
        delete (ObjectDataMap *)m_objectsData;
    }
}

ObjectLocalName
ShareGroup::genName(GenNameInfo genNameInfo,
                    ObjectLocalName p_localName,
                    bool genLocal)
{
    if (toIndex(genNameInfo.m_type) >=
        toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        return 0;
    }

    emugl::Mutex::AutoLock lock(m_namespaceLock);
    ObjectLocalName localName =
            m_nameSpace[toIndex(genNameInfo.m_type)]->genName(
                                                    genNameInfo,
                                                    p_localName, genLocal);
    return localName;
}

ObjectLocalName ShareGroup::genName(NamedObjectType namedObjectType,
                                    ObjectLocalName p_localName,
                                    bool genLocal) {
    return genName(GenNameInfo(namedObjectType), p_localName, genLocal);
}

ObjectLocalName ShareGroup::genName(ShaderProgramType shaderProgramType,
                                    ObjectLocalName p_localName,
                                    bool genLocal,
                                    GLuint existingGlobal) {
    return genName(GenNameInfo(shaderProgramType, existingGlobal), p_localName, genLocal);
}

unsigned int
ShareGroup::getGlobalName(NamedObjectType p_type,
                          ObjectLocalName p_localName)
{
    if (toIndex(p_type) >= toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        return 0;
    }

    emugl::Mutex::AutoLock lock(m_namespaceLock);
    return m_nameSpace[toIndex(p_type)]->getGlobalName(p_localName);
}

ObjectLocalName
ShareGroup::getLocalName(NamedObjectType p_type,
                         unsigned int p_globalName)
{
    if (toIndex(p_type) >= toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        return 0;
    }

    emugl::Mutex::AutoLock lock(m_namespaceLock);
    return m_nameSpace[toIndex(p_type)]->getLocalName(p_globalName);
}

NamedObjectPtr ShareGroup::getNamedObject(NamedObjectType p_type,
                                          ObjectLocalName p_localName) {
    if (toIndex(p_type) >= toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        return 0;
    }

    emugl::Mutex::AutoLock lock(m_namespaceLock);
    return m_nameSpace[toIndex(p_type)]->getNamedObject(p_localName);
}

void
ShareGroup::deleteName(NamedObjectType p_type, ObjectLocalName p_localName)
{
    if (toIndex(p_type) >= toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        return;
    }

    {
        emugl::Mutex::AutoLock lock(m_namespaceLock);
        m_nameSpace[toIndex(p_type)]->deleteName(p_localName);
    }

    ObjectDataAutoLock lock(this);
    ObjectDataMap *map = (ObjectDataMap *)m_objectsData;
    if (map) {
        (*map)[toIndex(p_type)].erase(p_localName);
    }
}

bool
ShareGroup::isObject(NamedObjectType p_type, ObjectLocalName p_localName)
{
    if (toIndex(p_type) >= toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        return 0;
    }

    emugl::Mutex::AutoLock lock(m_namespaceLock);
    return m_nameSpace[toIndex(p_type)]->isObject(p_localName);
}

void
ShareGroup::replaceGlobalObject(NamedObjectType p_type,
                              ObjectLocalName p_localName,
                              NamedObjectPtr p_globalObject)
{
    if (toIndex(p_type) >= toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        return;
    }

    emugl::Mutex::AutoLock lock(m_namespaceLock);
    m_nameSpace[toIndex(p_type)]->replaceGlobalObject(p_localName,
                                                               p_globalObject);
}

void
ShareGroup::setObjectData(NamedObjectType p_type,
                          ObjectLocalName p_localName,
                          ObjectDataPtr data)
{
    if (toIndex(p_type) >= toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        return;
    }

    ObjectDataAutoLock lock(this);

    ObjectDataMap *map = (ObjectDataMap *)m_objectsData;
    if (!map) {
        map = new ObjectDataMap();
        m_objectsData = map;
    }
    (*map)[toIndex(p_type)].emplace(p_localName, std::move(data));
}

static android::base::LazyInstance<ObjectDataPtr> nullObjectData = {};

const ObjectDataPtr& ShareGroup::getObjectDataPtrNoLock(
        NamedObjectType p_type, ObjectLocalName p_localName)
{
    if (const auto map = (ObjectDataMap*)m_objectsData) {
        const auto& typeMap = (*map)[toIndex(p_type)];
        const auto it = typeMap.find(p_localName);
        if (it != typeMap.end()) {
            return it->second;
        }
    }
    return *nullObjectData;
}

ObjectData* ShareGroup::getObjectData(NamedObjectType p_type,
                          ObjectLocalName p_localName)
{
    if (toIndex(p_type) >=
        toIndex(NamedObjectType::NUM_OBJECT_TYPES))
        return nullptr;

    ObjectDataAutoLock lock(this);
    return getObjectDataPtrNoLock(p_type, p_localName).get();
}

ObjectDataPtr ShareGroup::getObjectDataPtr(NamedObjectType p_type,
                          ObjectLocalName p_localName)
{
    if (toIndex(p_type) >=
        toIndex(NamedObjectType::NUM_OBJECT_TYPES))
        return {};

    ObjectDataAutoLock lock(this);
    return getObjectDataPtrNoLock(p_type, p_localName);
}

ObjectNameManager::ObjectNameManager(GlobalNameSpace *globalNameSpace) :
    m_globalNameSpace(globalNameSpace) {}

ShareGroupPtr
ObjectNameManager::createShareGroup(void *p_groupName, uint64_t sharedGroupID)
{
    emugl::Mutex::AutoLock lock(m_lock);

    ShareGroupPtr& shareGroupReturn = m_groups[p_groupName];
    if (!shareGroupReturn) {
        if (!sharedGroupID) {
            while (m_nextSharedGroupID == 0 ||
                   android::base::contains(m_usedSharedGroupIDs,
                                           m_nextSharedGroupID)) {
                m_nextSharedGroupID ++;
            }
            sharedGroupID = m_nextSharedGroupID;
            m_usedSharedGroupIDs.insert(sharedGroupID);
            ++m_nextSharedGroupID;
        } else {
            assert(!m_usedSharedGroupIDs.count(sharedGroupID));
        }
        shareGroupReturn.reset(
            new ShareGroup(m_globalNameSpace, sharedGroupID));
    } else {
        assert(sharedGroupID == 0
            || sharedGroupID == shareGroupReturn->getId());
    }

    return shareGroupReturn;
}

ShareGroupPtr
ObjectNameManager::getShareGroup(void *p_groupName)
{
    emugl::Mutex::AutoLock lock(m_lock);

    ShareGroupPtr shareGroupReturn;

    ShareGroupsMap::iterator s( m_groups.find(p_groupName) );
    if (s != m_groups.end()) {
        shareGroupReturn = (*s).second;
    }

    return shareGroupReturn;
}

ShareGroupPtr
ObjectNameManager::attachShareGroup(void *p_groupName,
                                    void *p_existingGroupName)
{
    emugl::Mutex::AutoLock lock(m_lock);

    ShareGroupsMap::iterator s( m_groups.find(p_existingGroupName) );
    if (s == m_groups.end()) {
        // ShareGroup is not found !!!
        return ShareGroupPtr();
    }

    ShareGroupPtr shareGroupReturn((*s).second);
    if (m_groups.find(p_groupName) == m_groups.end()) {
        m_groups.emplace(p_groupName, shareGroupReturn);
        m_usedSharedGroupIDs.insert(shareGroupReturn->getId());
    }
    return shareGroupReturn;
}

ShareGroupPtr ObjectNameManager::attachOrCreateShareGroup(void *p_groupName,
                                    uint64_t p_existingGroupID) {
    assert(m_groups.find(p_groupName) == m_groups.end());
    ShareGroupsMap::iterator ite = p_existingGroupID ? m_groups.begin()
                                                     : m_groups.end();
    while (ite != m_groups.end() && ite->second->getId() != p_existingGroupID) {
        ++ite;
    }
    if (ite == m_groups.end()) {
        return createShareGroup(p_groupName, p_existingGroupID);
    } else {
        return attachShareGroup(p_groupName, ite->first);
    }
}

void
ObjectNameManager::deleteShareGroup(void *p_groupName)
{
    emugl::Mutex::AutoLock lock(m_lock);
    auto sharedGroup = m_groups.find(p_groupName);
    if (sharedGroup == m_groups.end()) return;
    m_usedSharedGroupIDs.erase(sharedGroup->second->getId());
    m_groups.erase(sharedGroup);
}

void *ObjectNameManager::getGlobalContext()
{
    emugl::Mutex::AutoLock lock(m_lock);
    return m_groups.empty() ? nullptr : m_groups.begin()->first;
}
