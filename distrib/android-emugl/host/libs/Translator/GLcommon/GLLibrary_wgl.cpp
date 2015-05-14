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

#include <windows.h>

// TODO(digit): Remove this static C++ constructor. Doing so breaks stuff!!
static emugl::SharedLibrary* sLibGl = emugl::SharedLibrary::open("opengl32");

class WglLibrary : public GlLibrary {
public:
    virtual GlFunctionPointer findSymbol(const char* name) {
        // TODO(digit): Don't call wglGetProcAddress directly.
        GlFunctionPointer ret = (GlFunctionPointer)wglGetProcAddress(name);
        if (!ret && sLibGl) {
            ret = sLibGl->findSymbol(name);
        }
        return ret;
    }
};

static emugl::LazyInstance<WglLibrary> sHostInstance = LAZY_INSTANCE_INIT;

// static
GlLibrary* GlLibrary::getHostInstance() {
    return sHostInstance.ptr();
}
