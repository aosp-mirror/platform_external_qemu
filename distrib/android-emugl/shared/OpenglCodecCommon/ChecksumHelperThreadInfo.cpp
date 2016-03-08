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

#include "ChecksumHelperThreadInfo.h"

#include "emugl/common/lazy_instance.h"
#include "emugl/common/thread_store.h"

#include <stdio.h>
#include <string>
#include <atomic>

namespace {

class ChecksumHelperThreadStore : public ::emugl::ThreadStore {
public:
    ChecksumHelperThreadStore() : ::emugl::ThreadStore(&destructor) {}
#ifdef TRACE_CHECKSUMHELPER
    static std::atomic<size_t> mNumInstances;
#endif // TRACE_CHECKSUMHELPER

private:
    static void destructor(void* value) {
        LOG_CHECKSUMHELPER("%s: GLprotocol deleted %p (%lu instances)\n", __FUNCTION__,
                       value, (size_t)mNumInstances-1);
        delete static_cast<ChecksumHelperThreadInfo*>(value);
#ifdef TRACE_CHECKSUMHELPER
        mNumInstances--;
#endif // TRACE_CHECKSUMHELPER
    }
};

static ::emugl::LazyInstance<ChecksumHelperThreadStore> s_tls = LAZY_INSTANCE_INIT;

ChecksumHelperThreadInfo* getChecksumHelperThreadInfo() {
    ChecksumHelperThreadInfo *pti = static_cast<ChecksumHelperThreadInfo*>(s_tls->get());
    if (!pti) {
        pti = new ChecksumHelperThreadInfo();
#ifdef TRACE_CHECKSUMHELPER
	ChecksumHelperThreadStore::mNumInstances ++;
#endif // TRACE_CHECKSUMHELPER
        s_tls->set(pti);
        LOG_CHECKSUMHELPER("%s: GLprotocol created %p (%lu instances)\n", __FUNCTION__,
                       pti, (size_t)ChecksumHelperThreadStore::mNumInstances);
    }
    return pti;
}

}  // namespace

#ifdef TRACE_CHECKSUMHELPER
std::atomic<size_t> ChecksumHelperThreadStore::mNumInstances(0);
#endif // TRACE_CHECKSUMHELPER

uint32_t ChecksumHelperThreadInfo::getVersion() {
    return getChecksumHelperThreadInfo()->m_protocol.getVersion();
}

bool ChecksumHelperThreadInfo::setVersion(uint32_t version) {
    return getChecksumHelperThreadInfo()->m_protocol.setVersion(version);
}

bool ChecksumHelperThreadInfo::validate(void *buf, size_t packetLen) {
    return getChecksumHelperThreadInfo()->m_protocol.validate(buf, packetLen, (char *)buf + packetLen);
}

void ChecksumHelperThreadInfo::validOrDie(void *buf, size_t packetLen, const char* message) {
    // We should actually call crashhandler_die(message), but I don't think we
    // can link to that library from here
    if (!validate(buf, packetLen)) abort();
}
