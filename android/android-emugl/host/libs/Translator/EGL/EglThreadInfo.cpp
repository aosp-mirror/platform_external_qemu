/*
* Copyright (C) 2011 The Android Open Source Project
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
#include "EglThreadInfo.h"
#include "EglOsApi.h"

#include "emugl/common/lazy_instance.h"
#include "emugl/common/thread_store.h"

namespace {

class EglThreadInfoStore : public emugl::ThreadStore {
public:
    EglThreadInfoStore() : emugl::ThreadStore(&destructor) {}
private:
    static void destructor(void* value) {
        delete static_cast<EglThreadInfo*>(value);
    }
};

}  // namespace

EglThreadInfo::EglThreadInfo() :
        m_err(EGL_SUCCESS), m_api(EGL_OPENGL_ES_API) {}

static EglThreadInfoStore* s_tls = 0;
static thread_local EglThreadInfo* s_currInfo = 0;

// static
void EglThreadInfo::init() { if (!s_tls) s_tls = new EglThreadInfoStore; }

EglThreadInfo* EglThreadInfo::get(void)
{
    if (!s_currInfo) {
        EglThreadInfo *ti = static_cast<EglThreadInfo*>(s_tls->get());
        ti = new EglThreadInfo();
        s_tls->set(ti);
        s_currInfo = ti;
    }
    return s_currInfo;
}
