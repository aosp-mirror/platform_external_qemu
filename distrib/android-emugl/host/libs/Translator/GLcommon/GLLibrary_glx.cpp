// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "GLcommon/GLLibrary.h"

#include "emugl/common/lazy_instance.h"
#include "emugl/common/shared_library.h"

#include "OpenglCodecCommon/ErrorLog.h"

#include <GL/glx.h>

class GlxLibrary : public GlLibrary {
public:
    typedef GlFunctionPointer (ResolverFunc)(const char* name);

    // Important: Use libGL.so.1 explicitly, because it will always link to
    // the vendor-specific version of the library. libGL.so might in some
    // cases, depending on bad ldconfig configurations, link to the wrapper
    // lib that doesn't behave the same.
    GlxLibrary() : mLib(NULL), mResolver(NULL) {
        static const char kLibName[] = "libGL.so.1";
        char error[256];
        mLib = emugl::SharedLibrary::open(kLibName, error, sizeof(error));
        if (!mLib) {
            ERR("%s: Could not open GL library %s [%s]\n",
                __FUNCTION__, kLibName, error);
        }
        // NOTE: Don't use glXGetProcAddress here.
        static const char kResolverName[] = "glXGetProcAddressARB";
        mResolver = reinterpret_cast<ResolverFunc*>(
                mLib->findSymbol(kResolverName));
        if (!mResolver) {
            ERR("%s: Could not find resolver %s in %s\n",
                __FUNCTION__, kResolverName, kLibName);
            delete mLib;
            mLib = NULL;
        }
    }

    ~GlxLibrary() {
        delete mLib;
    }

    // override
    virtual GlFunctionPointer findSymbol(const char* name) {
        if (!mLib) {
            return NULL;
        }
        GlFunctionPointer ret = (*mResolver)(name);
        if (!ret) {
            ret = reinterpret_cast<GlFunctionPointer>(mLib->findSymbol(name));
        }
        return ret;
    }

private:
    emugl::SharedLibrary* mLib;
    ResolverFunc* mResolver;
};

static emugl::LazyInstance<GlxLibrary> sGlxLibrary = LAZY_INSTANCE_INIT;

// static
GlLibrary* GlLibrary::getHostInstance() {
    return sGlxLibrary.ptr();
}
