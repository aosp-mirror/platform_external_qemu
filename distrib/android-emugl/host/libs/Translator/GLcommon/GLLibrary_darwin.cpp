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

class DarwinGlLibrary : public GlLibrary {
public:
    DarwinGlLibrary() : mLib(NULL) {
        static const char kLibName[] =
                "/System/Library/Frameworks/OpenGL.framework/OpenGL";
        char error[256];
        mLib = emugl::SharedLibrary::open(kLibName, error, sizeof(error));
        if (!mLib) {
            ERR("%s: Could not open GL library %s [%s]\n",
                __FUNCTION__, kLibName, error);
        }
    }

    ~DarwinGlLibrary() {
        delete mLib;
    }

    // override
    virtual GlFunctionPointer findSymbol(const char* name) {
        if (!mLib) {
            return NULL;
        }
        return reinterpret_cast<GlFunctionPointer>(mLib->findSymbol(name));
    }

private:
    emugl::SharedLibrary* mLib;
};

static emugl::LazyInstance<DarwinGlLibrary> sDarwinGlLibrary =
        LAZY_INSTANCE_INIT;

// static
GlLibrary* GlLibrary::getHostInstance() {
    return sDarwinGlLibrary.ptr();
}
