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
#ifndef _OBJECT_NAME_MANAGER_H
#define _OBJECT_NAME_MANAGER_H

#include "emugl/common/mutex.h"
#include "emugl/common/smart_ptr.h"
#include "GLcommon/ObjectNameTypes.h"
#include <GLES/gl.h>
#include <unordered_map>

enum ObjectDataType {
    SHADER_DATA,
    PROGRAM_DATA,
    TEXTURE_DATA,
    BUFFER_DATA,
    UNDEFINED_DATA
};

class ObjectData
{
public:
    ObjectData(ObjectDataType type = UNDEFINED_DATA): m_dataType(type) {}
    ObjectDataType getDataType() { return m_dataType; };
    virtual ~ObjectData() = default;
private:
    ObjectDataType m_dataType;
};
typedef emugl::SmartPtr<ObjectData> ObjectDataPtr;

class GlobalNameSpace;
class NameSpace;

//
// class ShareGroup -
//   That class manages objects of one "local" context share group, typically
//   there will be one inctance of ShareGroup for each user OpenGL context
//   unless the user context share with another user context. In that case they
//   both will share the same ShareGroup instance.
//   calls into that class gets serialized through a lock so it is thread safe.
//
class ShareGroup
{
    friend class ObjectNameManager;
public:
    ~ShareGroup();

    //
    // genName - generates new object name and returns its name value.
    //           if genLocal is false, p_localName will be used as the name.
    //           This function also generates a "global" name for the object
    //           which can be queried using the getGlobalName function.
    ObjectLocalName genName(GenNameInfo genNameInfo, ObjectLocalName p_localName = 0, bool genLocal= false);
    //           overload for generating non-shader object
    ObjectLocalName genName(NamedObjectType namedObjectType, ObjectLocalName p_localName = 0, bool genLocal= false);
    //           overload for generating shader object
    ObjectLocalName genName(GLenum shaderType, ObjectLocalName p_localName = 0, bool genLocal= false);

    //
    // getGlobalName - retrieves the "global" name of an object or 0 if the
    //                 object does not exist.
    //
    unsigned int getGlobalName(NamedObjectType p_type, ObjectLocalName p_localName);

    //
    // getLocalName - retrieves the "local" name of an object or 0 if the
    //                 object does not exist.
    //
    ObjectLocalName getLocalName(NamedObjectType p_type, unsigned int p_globalName);

    //
    // deleteName - deletes and object from the namespace as well as its
    //              global name from the global name space.
    //
    void deleteName(NamedObjectType p_type, ObjectLocalName p_localName);

    //
    // replaceGlobalName - replaces an object to map to an existing global
    //        named object. (used when creating EGLImage siblings)
    //
    void replaceGlobalName(NamedObjectType p_type, ObjectLocalName p_localName, unsigned int p_globalName);

    //
    // isObject - returns true if the named object exist.
    //
    bool isObject(NamedObjectType p_type, ObjectLocalName p_localName);

    //
    // Assign object global data to a names object
    //
    void setObjectData(NamedObjectType p_type, ObjectLocalName p_localName, ObjectDataPtr data);

    //
    // Retrieve object global data
    //
    ObjectDataPtr getObjectData(NamedObjectType p_type, ObjectLocalName p_localName);

    //
    // Increase the reference counter of a texture by 1
    // Return the current counter
    //
    size_t incTexRefCounter(unsigned int p_globalName);

    //
    // Decrease the reference counter of a texture by 1
    // Return the current counter
    // Release texture if counter reaches 0
    size_t decTexRefCounterAndReleaseIf0(unsigned int p_globalName);

private:
    size_t incTexRefCounterNoLock(unsigned int p_globalName);

    explicit ShareGroup(GlobalNameSpace *globalNameSpace);

private:
    emugl::Mutex m_lock;
    NameSpace* m_nameSpace[static_cast<int>(NamedObjectType::NUM_OBJECT_TYPES)];
    void *m_objectsData = nullptr;
    void *m_globalTextureRefCounter = nullptr;
};

typedef emugl::SmartPtr<ShareGroup> ShareGroupPtr;
typedef std::unordered_map<void*, ShareGroupPtr> ShareGroupsMap;

//
// ObjectNameManager -
//   This class manages the set of all ShareGroups instances,
//   each ShareGroup instance can be accessed through one or more 'groupName'
//   values. the type of 'groupName' is void *, the intent is that the EGL
//   layer will use the user context handle as the name for its ShareGroup
//   object. Multiple names can be attached to a ShareGroup object to support
//   user context sharing.
//
class ObjectNameManager
{
public:
    explicit ObjectNameManager(GlobalNameSpace *globalNameSpace);

    //
    // createShareGroup - create a new ShareGroup object and attach it with
    //                    the "name" specified by p_groupName.
    //
    ShareGroupPtr createShareGroup(void *p_groupName);

    //
    // attachShareGroup - find the ShareGroup object attached to the name
    //    specified in p_existingGroupName and attach p_groupName to the same
    //    ShareGroup instance.
    //
    ShareGroupPtr attachShareGroup(void *p_groupName, void *p_existingGroupName);

    //
    // getShareGroup - retreive a ShareGroup object based on its "name"
    //
    ShareGroupPtr getShareGroup(void *p_groupName);

    //
    // deleteShareGroup - deletes the attachment of the p_groupName to its
    //           attached ShareGroup. When the last name of ShareGroup is
    //           deleted the ShareGroup object is destroyed.
    //
    void deleteShareGroup(void *p_groupName);

    //
    //  getGlobalContext() - this function returns a name of an existing
    //                       ShareGroup. The intent is that the EGL layer will
    //                       use that function to get the GL context which each
    //                       new context needs to share with.
    //
    void *getGlobalContext();

private:
    ShareGroupsMap m_groups;
    emugl::Mutex m_lock;
    GlobalNameSpace *m_globalNameSpace = nullptr;
};

#endif
