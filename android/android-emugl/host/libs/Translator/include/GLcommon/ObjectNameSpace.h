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

#include "aemu/base/containers/HybridComponentManager.h"
#include "snapshot/common.h"
#include "emugl/common/mutex.h"
#include "GLcommon/GLBackgroundLoader.h"
#include "GLcommon/NamedObject.h"
#include "GLcommon/ObjectData.h"
#include "GLcommon/SaveableTexture.h"
#include "GLcommon/TranslatorIfaces.h"

#include <GLES/gl.h>
#include <unordered_map>
#include <unordered_set>

typedef android::base::HybridComponentManager<10000, ObjectLocalName, NamedObjectPtr> NamesMap;
typedef std::unordered_map<ObjectLocalName, ObjectDataPtr> ObjectDataMap;
typedef android::base::HybridComponentManager<10000, unsigned int, ObjectLocalName> GlobalToLocalNamesMap;
typedef android::base::HybridComponentManager<10000, ObjectLocalName, bool> BoundAtLeastOnceMap;

class GlobalNameSpace;

//
// Class NameSpace - this class manages allocations and deletions of objects
//                   from a single "local" namespace (private to a context share
//                   group). For each allocated object name, a "global" name is
//                   generated as well to be used in the space where all
//                   contexts are shared.
//
//   NOTE: this class does not used by the EGL/GLES layer directly,
//         the EGL/GLES layer creates objects using the ShareGroup class
//         interface (see below).
class NameSpace
{
public:

    NameSpace(NamedObjectType p_type, GlobalNameSpace *globalNameSpace,
              android::base::Stream* stream,
              const ObjectData::loadObject_t& loadObject);
    ~NameSpace();

    //
    // genName - creates new object in the namespace and  returns its name.
    //           if genLocal is false then the specified p_localName will be used.
    //           This function also generate a global name for the object,
    //           the value of the global name can be retrieved using the
    //           getGlobalName function.
    //
    ObjectLocalName genName(GenNameInfo genNameInfo, ObjectLocalName p_localName, bool genLocal);

    //
    // getGlobalName - returns the global name of an object or 0 if the object
    //                 does not exist.
    //
    unsigned int getGlobalName(ObjectLocalName p_localName, bool* found = nullptr);

    //
    // getLocaalName - returns the local name of an object or 0 if the object
    //                 does not exist.
    //
    ObjectLocalName getLocalName(unsigned int p_globalName);

    //
    // getNamedObject - returns the shared pointer of an object or null if the
    //                  object does not exist.
    NamedObjectPtr getNamedObject(ObjectLocalName p_localName);

    //
    // deleteName - deletes and object from the namespace as well as its
    //              global name from the global name space.
    //
    void deleteName(ObjectLocalName p_localName);

    //
    // isObject - returns true if the named object exist.
    //
    bool isObject(ObjectLocalName p_localName);

    //
    // sets an object to map to an existing global object.
    //
    void setGlobalObject(ObjectLocalName p_localName,
            NamedObjectPtr p_namedObject);
    //
    // replaces an object to map to an existing global object
    //
    void replaceGlobalObject(ObjectLocalName p_localName, NamedObjectPtr p_namedObject);

    // sets that the local name has been bound at least once, to save time later
    void setBoundAtLeastOnce(ObjectLocalName p_localName);
    bool everBound(ObjectLocalName p_localName) const;

    const ObjectDataPtr& getObjectDataPtr(ObjectLocalName p_localName);
    void setObjectData(ObjectLocalName p_localName, ObjectDataPtr data);
    // snapshot functions
    void postLoad(const ObjectData::getObjDataPtr_t& getObjDataPtr);
    void postLoadRestore(const ObjectData::getGlobalName_t& getGlobalName);
    void preSave(GlobalNameSpace *globalNameSpace);
    void onSave(android::base::Stream* stream);
    ObjectDataMap::const_iterator objDataMapBegin() const;
    ObjectDataMap::const_iterator objDataMapEnd() const;
private:
    ObjectLocalName m_nextName = 0;
    NamesMap m_localToGlobalMap;
    ObjectDataMap m_objectDataMap;
    BoundAtLeastOnceMap m_boundMap;
    GlobalToLocalNamesMap m_globalToLocalMap;
    const NamedObjectType m_type;
    GlobalNameSpace *m_globalNameSpace = nullptr;
    // touchTextures loads all textures onto GPU
    // Please only call it if the NameSpace is for textures
    void touchTextures();
};

struct EglImage;
class TextureData;

// Class GlobalNameSpace - this class maintain all global GL object names.
//                         It is contained in the EglDisplay. One emulator has
//                         only one GlobalNameSpace.
class GlobalNameSpace
{
public:
    friend class NamedObject;

    // The following are used for snapshot
    void preSaveAddEglImage(EglImage* eglImage);
    void preSaveAddTex(TextureData* texture);
    void onSave(android::base::Stream* stream,
                const android::snapshot::ITextureSaverPtr& textureSaver,
                SaveableTexture::saver_t saver);
    void onLoad(android::base::Stream* stream,
                const android::snapshot::ITextureLoaderWPtr& textureLoaderWPtr,
                SaveableTexture::creator_t creator);
    void postLoad(android::base::Stream* stream);
    const SaveableTexturePtr& getSaveableTextureFromLoad(unsigned int oldGlobalName);
    SaveableTextureMap* getSaveableTextureMap() { return &m_textureMap; }

    void clearTextureMap();

    void setIfaces(const EGLiface* eglIface,
                   const GLESiface* glesIface) {
        m_eglIface = eglIface;
        m_glesIface = glesIface;
    }

private:

    emugl::Mutex m_lock;
    // m_textureMap is only used when saving / loading a snapshot
    // It is empty in all other situations
    SaveableTextureMap m_textureMap;

    std::shared_ptr<GLBackgroundLoader>     m_backgroundLoader;

    const EGLiface* m_eglIface = nullptr;
    const GLESiface* m_glesIface = nullptr;
};
