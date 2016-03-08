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

// Set LOG_GLPROTOCOL to 1 to debug creation/destruction of GLprotocol
// instances.
#define TRACE_GLPROTOCOL 0

#if TRACE_GLPROTOCOL
#define LOG_GLPROTOCOL(x...) fprintf(stderr, x)
#else
#define LOG_GLPROTOCOL(x...)
#endif

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

GLProtocolThreadInfo* getGLProtocol() {
    GLProtocolThreadInfo *protocol = static_cast<GLProtocolThreadInfo*>(s_tls->get());
    if (!protocol) {
        protocol = new GLProtocolThreadInfo();
	GLProtocolThreadStore::mNumInstances ++;
        s_tls->set(protocol);
        LOG_GLPROTOCOL("%s: GLprotocol created %p (%lu instances)\n", __FUNCTION__,
                       protocol, GLProtocolThreadStore::mNumInstances);
    }
    return protocol;
}

}  // namespace

size_t GLProtocolThreadStore::mNumInstances = 0;

uint32_t GLProtocolThreadInfo::getProtocol() {
    return getGLProtocol()->m_glProtocol;
}

void GLProtocolThreadInfo::setProtocol(uint32_t glProtocol) {
    getGLProtocol()->m_glProtocol = glProtocol;
}