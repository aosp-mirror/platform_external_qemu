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
#include "android/base/files/PathUtils.h"
#include "android/base/files/StreamSerializing.h"
#include "android/base/memory/LazyInstance.h"
#include "android/snapshot/TextureLoader.h"
#include "android/snapshot/TextureSaver.h"
#include "emugl/common/crash_reporter.h"
#include "emugl/common/logging.h"
#include "GLcommon/GLEScontext.h"
#include "GLcommon/TranslatorIfaces.h"

#include <assert.h>

using android::snapshot::ITextureSaver;
using android::snapshot::ITextureLoader;
using android::snapshot::ITextureSaverPtr;
using android::snapshot::ITextureLoaderPtr;
using android::snapshot::ITextureLoaderWPtr;

NameSpace::NameSpace(NamedObjectType p_type, GlobalNameSpace *globalNameSpace,
        android::base::Stream* stream, const ObjectData::loadObject_t& loadObject) :
    m_type(p_type),
    m_globalNameSpace(globalNameSpace) {
    if (!stream) return;
    // When loading from a snapshot, we restores translator states here, but
    // host GPU states are not touched until postLoadRestore is called.
    // GlobalNames are not yet generated.
    size_t objSize = stream->getBe32();
    for (size_t obj = 0; obj < objSize; obj++) {
        ObjectLocalName localName = stream->getBe64();
        ObjectDataPtr data = loadObject((NamedObjectType)m_type,
                localName, stream);
        if (m_type == NamedObjectType::TEXTURE) {
            // Texture data are managed differently
            // They are loaded by GlobalNameSpace before loading
            // share groups
            TextureData* texData = (TextureData*)data.get();
            if (!texData->getGlobalName()) {
                GL_LOG("NameSpace::%s: %p: texture data %p is 0 texture\n",
                       __func__, this, texData);
                continue;
            }

            SaveableTexturePtr saveableTexture =
                    globalNameSpace->getSaveableTextureFromLoad(
                            texData->getGlobalName());
            texData->setSaveableTexture(std::move(saveableTexture));
            texData->setGlobalName(0);
        }
        setObjectData(localName, std::move(data));
    }
}

NameSpace::~NameSpace() {
}

void NameSpace::postLoad(const ObjectData::getObjDataPtr_t& getObjDataPtr) {
    for (const auto& objData : m_objectDataMap) {
        GL_LOG("NameSpace::%s: %p: try to load object %llu\n", __func__, this, objData.first);
        if (!objData.second) {
            // bug: 130631787
            // emugl::emugl_crash_reporter(
            //         "Fatal: null object data ptr on restore\n");
            continue;
        }
        objData.second->postLoad(getObjDataPtr);
    }
}

void NameSpace::touchTextures() {
    assert(m_type == NamedObjectType::TEXTURE);
    for (const auto& obj : m_objectDataMap) {
        TextureData* texData = (TextureData*)obj.second.get();
        if (!texData->needRestore()) {
            GL_LOG("NameSpace::%s: %p: texture data %p does not need restore\n",
                    __func__, this, texData);
            continue;
        }
        const SaveableTexturePtr& saveableTexture = texData->getSaveableTexture();
        if (!saveableTexture.get()) {
            GL_LOG("NameSpace::%s: %p: warning: no saveableTexture for texture data %p\n",
                    __func__, this, texData);
            continue;
        }

        NamedObjectPtr texNamedObj = saveableTexture->getGlobalObject();
        if (!texNamedObj) {
            GL_LOG("NameSpace::%s: %p: fatal: global object null for texture data %p\n",
                    __func__, this, texData);
            emugl::emugl_crash_reporter(
                    "fatal: null global texture object in "
                    "NameSpace::touchTextures");
        }
        setGlobalObject(obj.first, texNamedObj);
        texData->setGlobalName(texNamedObj->getGlobalName());
        texData->restore(0, nullptr);
    }
}

void NameSpace::postLoadRestore(const ObjectData::getGlobalName_t& getGlobalName) {
    // Texture data are special, they got the global name from SaveableTexture
    // This is because texture data can be shared across multiple share groups
    if (m_type == NamedObjectType::TEXTURE) {
        touchTextures();
        return;
    }
    // 2 passes are needed for SHADER_OR_PROGRAM type, because (1) they
    // live in the same namespace (2) shaders must be created before
    // programs.
    int numPasses = m_type == NamedObjectType::SHADER_OR_PROGRAM
            ? 2 : 1;
    for (int pass = 0; pass < numPasses; pass ++) {
        for (const auto& obj : m_objectDataMap) {
            assert(m_type == ObjectDataType2NamedObjectType(
                    obj.second->getDataType()));
            // get global names
            if ((obj.second->getDataType() == PROGRAM_DATA && pass == 0)
                    || (obj.second->getDataType() == SHADER_DATA &&
                            pass == 1)) {
                continue;
            }
            genName(obj.second->getGenNameInfo(), obj.first, false);
            obj.second->restore(obj.first, getGlobalName);
        }
    }
}

void NameSpace::preSave(GlobalNameSpace *globalNameSpace) {
    if (m_type != NamedObjectType::TEXTURE) {
        return;
    }
    // In case we loaded textures from a previous snapshot and have not yet
    // restore them to GPU, we do the restoration here.
    // TODO: skip restoration and write saveableTexture directly to the new
    // snapshot
    touchTextures();
    for (const auto& obj : m_objectDataMap) {
        globalNameSpace->preSaveAddTex((TextureData*)obj.second.get());
    }
}

void NameSpace::onSave(android::base::Stream* stream) {
    stream->putBe32(m_objectDataMap.size());
    for (const auto& obj : m_objectDataMap) {
        stream->putBe64(obj.first);
        obj.second->onSave(stream, getGlobalName(obj.first));
    }
}


static android::base::LazyInstance<ObjectDataPtr> nullObjectData = {};
static android::base::LazyInstance<NamedObjectPtr> nullNamedObject = {};

ObjectLocalName
NameSpace::genName(GenNameInfo genNameInfo, ObjectLocalName p_localName, bool genLocal)
{
    assert(m_type == genNameInfo.m_type);
    ObjectLocalName localName = p_localName;
    if (genLocal) {
        do {
            localName = ++m_nextName;
        } while(localName == 0 ||
                nullptr != m_localToGlobalMap.getExceptZero_const(localName));
    }

    auto newObjPtr = NamedObjectPtr( new NamedObject(genNameInfo, m_globalNameSpace));
    m_localToGlobalMap.add(localName, newObjPtr);

    unsigned int globalName = newObjPtr->getGlobalName();
    m_globalToLocalMap.add(globalName, localName);
    return localName;
}


unsigned int
NameSpace::getGlobalName(ObjectLocalName p_localName, bool* found)
{
    auto objPtrPtr = m_localToGlobalMap.getExceptZero_const(p_localName);

    if (!objPtrPtr) {
        if (found) *found = false;
        return 0;
    }

    if (found) *found = true;
    auto res =  (*objPtrPtr)->getGlobalName();
    return res;
}

ObjectLocalName
NameSpace::getLocalName(unsigned int p_globalName)
{
    auto localPtr = m_globalToLocalMap.get_const(p_globalName);
    if (!localPtr) return 0;
    return *localPtr;
}

NamedObjectPtr NameSpace::getNamedObject(ObjectLocalName p_localName) {
    auto objPtrPtr = m_localToGlobalMap.get_const(p_localName);
    if (!objPtrPtr || !(*objPtrPtr)) return nullptr;
    return *objPtrPtr;
}

void
NameSpace::deleteName(ObjectLocalName p_localName)
{
    auto objPtrPtr = m_localToGlobalMap.getExceptZero(p_localName);
    if (objPtrPtr) {
        m_globalToLocalMap.remove((*objPtrPtr)->getGlobalName());
        *objPtrPtr = nullNamedObject.get();
        m_localToGlobalMap.remove(p_localName);
    }

    m_objectDataMap.erase(p_localName);
    m_boundMap.remove(p_localName);
}

bool
NameSpace::isObject(ObjectLocalName p_localName)
{
    auto objPtrPtr = m_localToGlobalMap.getExceptZero_const(p_localName);
    return nullptr != objPtrPtr;
}

void
NameSpace::setGlobalObject(ObjectLocalName p_localName,
                               NamedObjectPtr p_namedObject) {

    auto objPtrPtr = m_localToGlobalMap.getExceptZero(p_localName);
    if (objPtrPtr) {
        m_globalToLocalMap.remove((*objPtrPtr)->getGlobalName());
        *objPtrPtr = p_namedObject;
    } else {
        m_localToGlobalMap.add(p_localName, p_namedObject);
    }

    m_globalToLocalMap.add(p_namedObject->getGlobalName(), p_localName);
}

void
NameSpace::replaceGlobalObject(ObjectLocalName p_localName,
                               NamedObjectPtr p_namedObject)
{
    auto objPtrPtr = m_localToGlobalMap.getExceptZero(p_localName);
    if (objPtrPtr) {
        m_globalToLocalMap.remove((*objPtrPtr)->getGlobalName());
        *objPtrPtr = p_namedObject;
        m_globalToLocalMap.add(p_namedObject->getGlobalName(), p_localName);
    }
}

// sets that the local name has been bound at least once, to save time later
void NameSpace::setBoundAtLeastOnce(ObjectLocalName p_localName) {
    m_boundMap.add(p_localName, true);
}

// sets that the local name has been bound at least once, to save time later
bool NameSpace::everBound(ObjectLocalName p_localName) const {
    const bool* boundPtr = m_boundMap.get_const(p_localName);
    if (!boundPtr) return false;
    return *boundPtr;
}

ObjectDataMap::const_iterator NameSpace::objDataMapBegin() const {
    return m_objectDataMap.begin();
}

ObjectDataMap::const_iterator NameSpace::objDataMapEnd() const {
    return m_objectDataMap.end();
}

const ObjectDataPtr& NameSpace::getObjectDataPtr(
        ObjectLocalName p_localName) {
    const auto it = m_objectDataMap.find(p_localName);
    if (it != m_objectDataMap.end()) {
        return it->second;
    }
    return *nullObjectData;
}

void NameSpace::setObjectData(ObjectLocalName p_localName,
        ObjectDataPtr data) {
    m_objectDataMap[p_localName] = std::move(data);
}

void GlobalNameSpace::preSaveAddEglImage(EglImage* eglImage) {
    if (!eglImage->globalTexObj) {
        GL_LOG("GlobalNameSpace::%s: %p: egl image %p with null texture object\n",
               __func__, this, eglImage);
        emugl::emugl_crash_reporter(
                "Fatal: egl image with null texture object\n");
    }
    unsigned int globalName = eglImage->globalTexObj->getGlobalName();
    emugl::Mutex::AutoLock lock(m_lock);

    if (!globalName) {
        GL_LOG("GlobalNameSpace::%s: %p: egl image %p has 0 texture object\n",
               __func__, this, eglImage);
        return;
    }

    const auto& saveableTexIt = m_textureMap.find(globalName);
    if (saveableTexIt == m_textureMap.end()) {
        assert(eglImage->saveableTexture);
        m_textureMap.emplace(globalName, eglImage->saveableTexture);
    } else {
        assert(m_textureMap[globalName] == eglImage->saveableTexture);
    }
}

void GlobalNameSpace::preSaveAddTex(TextureData* texture) {
    emugl::Mutex::AutoLock lock(m_lock);
    const auto& saveableTexIt = m_textureMap.find(texture->getGlobalName());

    if (!texture->getGlobalName()) {
        GL_LOG("GlobalNameSpace::%s: %p: texture data %p is 0 texture\n",
               __func__, this, texture);
        return;
    }

    if (saveableTexIt == m_textureMap.end()) {
        assert(texture->getSaveableTexture());
        m_textureMap.emplace(texture->getGlobalName(),
                             texture->getSaveableTexture());
    } else {
        assert(m_textureMap[texture->getGlobalName()] ==
               texture->getSaveableTexture());
    }
}

void GlobalNameSpace::onSave(android::base::Stream* stream,
                             const ITextureSaverPtr& textureSaver,
                             SaveableTexture::saver_t saver) {
#if SNAPSHOT_PROFILE > 1
    int cleanTexs = 0;
    int dirtyTexs = 0;
#endif // SNAPSHOT_PROFILE > 1
    saveCollection(
            stream, m_textureMap,
            [saver, &textureSaver
#if SNAPSHOT_PROFILE > 1
            , &cleanTexs, &dirtyTexs
#endif // SNAPSHOT_PROFILE > 1
                ](
                    android::base::Stream* stream,
                    const std::pair<const unsigned int, SaveableTexturePtr>&
                            tex) {
                stream->putBe32(tex.first);
#if SNAPSHOT_PROFILE > 1
                if (tex.second.get() && tex.second->isDirty()) {
                    dirtyTexs ++;
                } else {
                    cleanTexs ++;
                }
#endif // SNAPSHOT_PROFILE > 1
                textureSaver->saveTexture(
                        tex.first,
                        [saver, &tex](android::base::Stream* stream,
                                      ITextureSaver::Buffer* buffer) {
                            if (!tex.second.get()) return;
                            saver(tex.second.get(), stream, buffer);
                        });
            });
    clearTextureMap();
#if SNAPSHOT_PROFILE > 1
    printf("Dirty texture saved %d, clean texture saved %d\n",
            dirtyTexs, cleanTexs);
#endif // SNAPSHOT_PROFILE > 1
}

void GlobalNameSpace::onLoad(android::base::Stream* stream,
                             const ITextureLoaderWPtr& textureLoaderWPtr,
                             SaveableTexture::creator_t creator) {
    const ITextureLoaderPtr textureLoader = textureLoaderWPtr.lock();
    assert(m_textureMap.size() == 0);
    if (!textureLoader->start()) {
        fprintf(stderr,
                "Error: texture file unsupported version or corrupted.\n");
        emugl::emugl_crash_reporter(
                "Error: texture file unsupported version or corrupted.\n");
        return;
    }
    loadCollection(
            stream, &m_textureMap,
            [this, creator, textureLoaderWPtr](android::base::Stream* stream) {
                unsigned int globalName = stream->getBe32();
                // A lot of function wrapping happens here.
                // When touched, saveableTexture triggers
                // textureLoader->loadTexture, which sets up the file position
                // and the mutex, and triggers saveableTexture->loadFromStream
                // for the real loading.
                SaveableTexture* saveableTexture = creator(
                        this, [globalName, textureLoaderWPtr](
                                      SaveableTexture* saveableTexture) {
                            auto textureLoader = textureLoaderWPtr.lock();
                            if (!textureLoader) return;
                            textureLoader->loadTexture(
                                    globalName,
                                    [saveableTexture](
                                            android::base::Stream* stream) {
                                        saveableTexture->loadFromStream(stream);
                                    });
                        });
                return std::make_pair(globalName,
                                      SaveableTexturePtr(saveableTexture));
            });

    m_backgroundLoader =
        std::make_shared<GLBackgroundLoader>(
            textureLoaderWPtr, *m_eglIface, *m_glesIface, m_textureMap);
    textureLoader->acquireLoaderThread(m_backgroundLoader);
}

void GlobalNameSpace::clearTextureMap() {
    decltype(m_textureMap)().swap(m_textureMap);
}

void GlobalNameSpace::postLoad(android::base::Stream* stream) {
    m_backgroundLoader->start();
    m_backgroundLoader.reset(); // leave it to TextureLoader
}

const SaveableTexturePtr& GlobalNameSpace::getSaveableTextureFromLoad(
        unsigned int oldGlobalName) {
    assert(m_textureMap.count(oldGlobalName));
    return m_textureMap[oldGlobalName];
}
