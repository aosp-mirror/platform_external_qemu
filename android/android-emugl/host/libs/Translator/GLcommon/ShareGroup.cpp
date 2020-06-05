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
#include <GLcommon/ShareGroup.h>
#include <GLcommon/ObjectNameSpace.h>
#include <GLcommon/GLEScontext.h>

#include "android/base/containers/Lookup.h"
#include "GLcommon/FramebufferData.h"

#include "emugl/common/logging.h"

#include <array>
#include <utility>

static constexpr int toIndex(NamedObjectType type) {
    return static_cast<int>(type);
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

ShareGroup::ShareGroup(GlobalNameSpace *globalNameSpace,
                       uint64_t sharedGroupID,
                       android::base::Stream* stream,
                       const ObjectData::loadObject_t& loadObject) :
                       m_sharedGroupID(sharedGroupID) {
    ObjectDataAutoLock lock(this);
    for (int i = 0; i < toIndex(NamedObjectType::NUM_OBJECT_TYPES);
         i++) {
        m_nameSpace[i] = new NameSpace(static_cast<NamedObjectType>(i),
                                       globalNameSpace, stream, loadObject);
    }
    if (stream) {
        m_needLoadRestore = true;
        int i = 0;
        for (auto ns : m_nameSpace) {
            GL_LOG("ShareGroup::%s: %p: start restore namespace for type %d\n", __func__, this, i);
            ns->postLoad(
                    [this](NamedObjectType p_type, ObjectLocalName p_localName) {
                        return this->getObjectDataPtrNoLock(p_type, p_localName);
                });
            GL_LOG("ShareGroup::%s: %p: finish restore namespace for type %d\n", __func__, this, i);
            ++i;
        }
    }
}

void ShareGroup::preSave(GlobalNameSpace *globalNameSpace) {
    ObjectDataAutoLock lock(this);
    if (m_saveStage == PreSaved) return;
    assert(m_saveStage == Empty);
    m_saveStage = PreSaved;
    m_nameSpace[(int)NamedObjectType::TEXTURE]->preSave(globalNameSpace);
}

void ShareGroup::onSave(android::base::Stream* stream) {
    // we do not save m_nameSpace
    ObjectDataAutoLock lock(this);
    if (m_saveStage == Saved) return;
    assert(m_saveStage == PreSaved);
    m_saveStage = Saved;
    int i = 0;
    for (auto ns : m_nameSpace) {
        GL_LOG("ShareGroup::%s: %p: start saving type %d\n", __func__, this, i);
        ns->onSave(stream);
        GL_LOG("ShareGroup::%s: %p: finish saving type %d\n", __func__, this, i);
        ++i;
    }
}

void ShareGroup::postSave(android::base::Stream* stream) {
    (void)stream;
    m_saveStage = Empty;
    // We need to mark the textures dirty, for those that has been bound to
    // a potential render target.
    NameSpace* renderbufferNs = m_nameSpace[(int)NamedObjectType::RENDERBUFFER];
    for (ObjectDataMap::const_iterator it = renderbufferNs->objDataMapBegin();
        it != renderbufferNs->objDataMapEnd();
        it ++) {
        RenderbufferData* rbData = (RenderbufferData*)it->second.get();
        rbData->makeTextureDirty();
    }
}

void ShareGroup::postLoadRestore() {
    emugl::Mutex::AutoLock lock(m_restoreLock);
    if (m_needLoadRestore) {
        int i = 0;
        for (auto ns : m_nameSpace) {
            GL_LOG("ShareGroup::%s: %p: start post load restore namespace for type %d\n", __func__, this, i);
            ns->postLoadRestore([this](NamedObjectType p_type,
                                ObjectLocalName p_localName) {
                                    return getGlobalName(p_type, p_localName);
                            });
            GL_LOG("ShareGroup::%s: %p: end post load restore namespace for type %d\n", __func__, this, i);
            ++i;
        }
        m_needLoadRestore = false;
    }
}

bool ShareGroup::needRestore() {
    return m_needLoadRestore;
}

void ShareGroup::lockObjectData() {
    while (m_objectsDataLock.test_and_set(std::memory_order_acquire)) {
        ;
    }
}

void ShareGroup::unlockObjectData() {
    m_objectsDataLock.clear(std::memory_order_release);
}

ShareGroup::~ShareGroup()
{
    {
        emugl::Mutex::AutoLock lock(m_namespaceLock);
        ObjectDataAutoLock objDataLock(this);
        for (auto n : m_nameSpace) {
            delete n;
        }
    }
}

ObjectLocalName
ShareGroup::genName(GenNameInfo genNameInfo,
                    ObjectLocalName p_localName,
                    bool genLocal)
{
    assert(genNameInfo.m_type != NamedObjectType::FRAMEBUFFER);
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
    assert(p_type != NamedObjectType::FRAMEBUFFER);
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
    assert(p_type != NamedObjectType::FRAMEBUFFER);
    if (toIndex(p_type) >= toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        return 0;
    }

    emugl::Mutex::AutoLock lock(m_namespaceLock);
    return m_nameSpace[toIndex(p_type)]->getLocalName(p_globalName);
}

NamedObjectPtr ShareGroup::getNamedObject(NamedObjectType p_type,
                                          ObjectLocalName p_localName) {
    assert(p_type != NamedObjectType::FRAMEBUFFER);
    if (toIndex(p_type) >= toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        return 0;
    }

    emugl::Mutex::AutoLock lock(m_namespaceLock);
    return m_nameSpace[toIndex(p_type)]->getNamedObject(p_localName);
}

void
ShareGroup::deleteName(NamedObjectType p_type, ObjectLocalName p_localName)
{
    assert(p_type != NamedObjectType::FRAMEBUFFER);
    if (toIndex(p_type) >= toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        return;
    }

    emugl::Mutex::AutoLock lock(m_namespaceLock);
    ObjectDataAutoLock objDataLock(this);
    m_nameSpace[toIndex(p_type)]->deleteName(p_localName);
}

bool
ShareGroup::isObject(NamedObjectType p_type, ObjectLocalName p_localName)
{
    assert(p_type != NamedObjectType::FRAMEBUFFER);
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
    assert(p_type != NamedObjectType::FRAMEBUFFER);
    if (toIndex(p_type) >= toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        return;
    }

    emugl::Mutex::AutoLock lock(m_namespaceLock);
    m_nameSpace[toIndex(p_type)]->replaceGlobalObject(p_localName,
                                                               p_globalObject);
}

void
ShareGroup::setGlobalObject(NamedObjectType p_type,
                              ObjectLocalName p_localName,
                              NamedObjectPtr p_globalObject)
{
    assert(p_type != NamedObjectType::FRAMEBUFFER);
    if (toIndex(p_type) >= toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        return;
    }

    emugl::Mutex::AutoLock lock(m_namespaceLock);
    m_nameSpace[toIndex(p_type)]->setGlobalObject(p_localName,
                                                  p_globalObject);
}

void
ShareGroup::setObjectData(NamedObjectType p_type,
                          ObjectLocalName p_localName,
                          ObjectDataPtr data) {
    ObjectDataAutoLock lock(this);
    setObjectDataLocked(p_type, p_localName, std::move(data));
}

void
ShareGroup::setObjectDataLocked(NamedObjectType p_type,
                          ObjectLocalName p_localName,
                          ObjectDataPtr&& data)
{
    assert(p_type != NamedObjectType::FRAMEBUFFER);
    if (toIndex(p_type) >= toIndex(NamedObjectType::NUM_OBJECT_TYPES)) {
        return;
    }
    m_nameSpace[toIndex(p_type)]->setObjectData(p_localName, data);
}

const ObjectDataPtr& ShareGroup::getObjectDataPtrNoLock(
        NamedObjectType p_type, ObjectLocalName p_localName) {
    assert(p_type != NamedObjectType::FRAMEBUFFER);
    return m_nameSpace[toIndex(p_type)]->getObjectDataPtr(p_localName);
}

ObjectData* ShareGroup::getObjectData(NamedObjectType p_type,
                          ObjectLocalName p_localName) {
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

#define CC_LIKELY( exp )    (__builtin_expect( !!(exp), true ))
#define CC_UNLIKELY( exp )  (__builtin_expect( !!(exp), false ))

unsigned int ShareGroup::ensureObjectOnBind(NamedObjectType p_type, ObjectLocalName p_localName) {
    emugl::Mutex::AutoLock lock(m_namespaceLock);

    auto ns = m_nameSpace[toIndex(p_type)];

    bool isObj;
    unsigned int globalName = ns->getGlobalName(p_localName, &isObj);

    if (CC_LIKELY(isObj)) {
        bool everBound = ns->everBound(p_localName);
        if (CC_LIKELY(everBound)) return globalName;

        auto ptr = ns->getObjectDataPtr(p_localName);

        if (ptr) {
            switch (p_type) {
                case NamedObjectType::VERTEXBUFFER: {
                    auto vbo = ((GLESbuffer*)(ptr.get()));
                    vbo->setBinded();
                    break;
                }
                // TODO: Add other object types here
                default:
                    fprintf(stderr, "%s: Warning: Unhandled object type 0x%x\n",
                            __func__, (uint32_t)p_type);
                    break;
            }
        }

        ns->setBoundAtLeastOnce(p_localName);
        return globalName;
    }

    // No such object, generate one and bind it
    bool genLocal = false;
    auto gi = GenNameInfo(p_type);
    ObjectLocalName localName =
            ns->genName( gi, p_localName, genLocal);

    switch (p_type) {
        case NamedObjectType::VERTEXBUFFER: {
            auto vbo = new GLESbuffer;
            vbo->setBinded();
            ns->setObjectData(p_localName, ObjectDataPtr(vbo));
            break;
        }
        // TODO: Add other object types here
        default:
            fprintf(stderr, "%s: Warning: Unhandled object type 0x%x\n",
                    __func__, (uint32_t)p_type);
            break;
    }

    ns->setBoundAtLeastOnce(p_localName);
    return (uint32_t)(ns->getGlobalName(p_localName));
}

ObjectNameManager::ObjectNameManager(GlobalNameSpace *globalNameSpace) :
    m_globalNameSpace(globalNameSpace) {}

ShareGroupPtr
ObjectNameManager::createShareGroup(void *p_groupName, uint64_t sharedGroupID,
        android::base::Stream* stream, const ObjectData::loadObject_t& loadObject)
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
            m_usedSharedGroupIDs.insert(sharedGroupID);
        }
        shareGroupReturn.reset(new ShareGroup(m_globalNameSpace, sharedGroupID,
                                              stream, loadObject));
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
        uint64_t p_existingGroupID, android::base::Stream* stream,
        const ObjectData::loadObject_t& loadObject) {
    assert(m_groups.find(p_groupName) == m_groups.end());
    ShareGroupsMap::iterator ite = p_existingGroupID ? m_groups.begin()
                                                     : m_groups.end();
    while (ite != m_groups.end() && ite->second->getId() != p_existingGroupID) {
        ++ite;
    }
    if (ite == m_groups.end()) {
        return createShareGroup(p_groupName, p_existingGroupID, stream,
                                loadObject);
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

void ObjectNameManager::preSave() {
    for (auto& shareGroup : m_groups) {
        shareGroup.second->preSave(m_globalNameSpace);
    }
}
