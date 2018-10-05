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

#include "ThreadInfo.h"

#include "emugl/common/lazy_instance.h"
#include "emugl/common/thread_store.h"

#include <stdio.h>

// Set TRACE_THREADINFO to 1 to debug creation/destruction of ThreadInfo
// instances.
#define TRACE_THREADINFO 0

#if TRACE_THREADINFO
#include <atomic>
#define LOG_THREADINFO(x...) fprintf(stderr, x)
using InstanceCounter = std::atomic<size_t>;
#endif

namespace {

class ThreadInfoStore : public ::emugl::ThreadStore {
public:
    ThreadInfoStore() : ::emugl::ThreadStore(&destructor) {}

    void set(void* value) {
        emugl::ThreadStore::set(value);
#if TRACE_THREADINFO
        ++mNumInstances;
#endif
    }

#if TRACE_THREADINFO
    static size_t getInstanceCount() { return mNumInstances; }
#endif

private:
    static void destructor(void* value) {
        delete static_cast<ThreadInfo*>(value);
#if TRACE_THREADINFO
        LOG_THREADINFO("%s: EFL %p (%d instances)\n", __FUNCTION__,
                       value, (int)mNumInstances);
        mNumInstances--;
#endif
    }

#if TRACE_THREADINFO
    static InstanceCounter mNumInstances;
#endif
};

#if TRACE_THREADINFO
InstanceCounter ThreadInfoStore::mNumInstances { 0 };
#endif

}  // namespace


void ThreadInfo::updateInfo(ContextPtr eglCtx,
                            EglDisplay* dpy,
                            GLEScontext* glesCtx,
                            ShareGroupPtr share,
                            ObjectNameManager* manager) {

    eglContext  = eglCtx;
    eglDisplay  = dpy;
    glesContext = glesCtx;
    shareGroup  = share;
    objManager  = manager;
}

static ::emugl::LazyInstance<ThreadInfoStore> s_tls = LAZY_INSTANCE_INIT;

ThreadInfo *getThreadInfo()
{
    ThreadInfo *ti = static_cast<ThreadInfo*>(s_tls->get());
    if (!ti) {
        ti = new ThreadInfo();
        s_tls->set(ti);
#if TRACE_THREADINFO
        LOG_THREADINFO("%s: EGL %p (%d instances)\n", __FUNCTION__,
                       ti, (int)ThreadInfoStore::getInstanceCount());
#endif
    }
    return ti;
}
