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

#include "GLProtocolThreadInfo.h"

#include "emugl/common/lazy_instance.h"
#include "emugl/common/thread_store.h"

#include <stdio.h>
#include <string>

namespace {

class GLProtocolThreadStore : public ::emugl::ThreadStore {
public:
    GLProtocolThreadStore() : ::emugl::ThreadStore(&destructor) {}
    static size_t mNumInstances;

private:
    static void destructor(void* value) {
        LOG_GLPROTOCOL("%s: GLprotocol deleted %p (%lu instances)\n", __FUNCTION__,
                       value, mNumInstances);
        delete static_cast<GLProtocolThreadInfo*>(value);
        mNumInstances--;
    }
};

static ::emugl::LazyInstance<GLProtocolThreadStore> s_tls = LAZY_INSTANCE_INIT;

GLProtocolThreadInfo* getGLProtocolThreadInfo() {
    GLProtocolThreadInfo *pti = static_cast<GLProtocolThreadInfo*>(s_tls->get());
    if (!pti) {
        pti = new GLProtocolThreadInfo();
	GLProtocolThreadStore::mNumInstances ++;
        s_tls->set(pti);
        LOG_GLPROTOCOL("%s: GLprotocol created %p (%lu instances)\n", __FUNCTION__,
                       pti, GLProtocolThreadStore::mNumInstances);
    }
    return pti;
}

}  // namespace

size_t GLProtocolThreadStore::mNumInstances = 0;

uint32_t GLProtocolThreadInfo::getVersion() {
    return getGLProtocolThreadInfo()->m_protocol.getVersion();
}

bool GLProtocolThreadInfo::setVersion(uint32_t version) {
    return getGLProtocolThreadInfo()->m_protocol.setVersion(version);
}

bool GLProtocolThreadInfo::validate(void *buf, size_t packetLen) {
    return getGLProtocolThreadInfo()->m_protocol.validate(buf, packetLen);
}

void GLProtocolThreadInfo::validOrDie(void *buf, size_t packetLen, const char* message) {
    // We should actually call crashhandler_die(message), but I don't think we
    // can link to that library from here
    if (!validate(buf, packetLen)) abort();
}
