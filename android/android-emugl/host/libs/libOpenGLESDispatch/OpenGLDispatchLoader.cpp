// Copyright (C) 2017 The Android Open Source Project
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

#include "emugl/common/OpenGLDispatchLoader.h"
#include "emugl/common/shared_library.h"

#include "android/base/containers/Lookup.h"
#include "android/base/memory/LazyInstance.h"

#include "OpenGLESDispatch/DispatchTables.h"

#include <string>
#include <unordered_map>

GLESv1Dispatch s_gles1;
GLESv2Dispatch s_gles2;

using android::base::LazyInstance;

// Must be declared outside of LazyLoaded*Dispatch scope due to the use of
// sizeof(T) within the template definition.
LazyInstance<LazyLoadedGLESv1Dispatch> sGLESv1Dispatch = LAZY_INSTANCE_INIT;
LazyInstance<LazyLoadedGLESv2Dispatch> sGLESv2Dispatch = LAZY_INSTANCE_INIT;
LazyInstance<LazyLoadedEGLDispatch> sEGLDispatch = LAZY_INSTANCE_INIT;

// Class that holds all dispatches we wanted to get so far.
class DispatchStore {
public:
    DispatchStore() = default;

    ~DispatchStore() {
        for (auto it : mEGLDispatches) {
            delete it.second;
        }
        for (auto it : mGLESv1Dispatches) {
            delete it.second;
        }
        for (auto it : mGLESv2Dispatches) {
            delete it.second;
        }
    }

    EGLDispatch* loadEGL(emugl::SharedLibrary* lib) {

        if (auto table = android::base::findOrDefault(mEGLDispatches,
                                                      lib,
                                                      nullptr)) {
            return table;
        }

        EGLDispatch dispatch;
        if (init_egl_dispatch_from_lib(lib, &dispatch)) {
            EGLDispatch* allocatedDispatch = new EGLDispatch;
            mEGLDispatches[lib] = allocatedDispatch;
            memcpy(allocatedDispatch, &dispatch, sizeof(EGLDispatch));
            return allocatedDispatch;
        } else {
            mEGLDispatches[lib] = nullptr;
            return nullptr;
        }
    }

    GLESv1Dispatch* loadGLESv1(emugl::SharedLibrary* lib, EGLDispatch* egl_dispatch) {
        if (auto table = android::base::findOrDefault(mGLESv1Dispatches, lib, nullptr)) {
            return table;
        }

        GLESv1Dispatch dispatch;
        if (gles1_dispatch_init_from_lib(lib, egl_dispatch, &dispatch)) {
            GLESv1Dispatch* allocatedDispatch = new GLESv1Dispatch;
            mGLESv1Dispatches[lib] = allocatedDispatch;
            memcpy(allocatedDispatch, &dispatch, sizeof(GLESv1Dispatch));
            return allocatedDispatch;
        } else {
            mGLESv1Dispatches[lib] = nullptr;
            return nullptr;
        }
    }

    GLESv2Dispatch* loadGLESv2(emugl::SharedLibrary* lib, EGLDispatch* egl_dispatch) {
        if (auto table = android::base::findOrDefault(mGLESv2Dispatches, lib, nullptr)) {
            return table;
        }

        GLESv2Dispatch dispatch;
        if (gles2_dispatch_init_from_lib(lib, egl_dispatch, &dispatch, true)) {
            GLESv2Dispatch* allocatedDispatch = new GLESv2Dispatch;
            mGLESv2Dispatches[lib] = allocatedDispatch;
            memcpy(allocatedDispatch, &dispatch, sizeof(GLESv2Dispatch));
            return allocatedDispatch;
        } else {
            mGLESv2Dispatches[lib] = nullptr;
            return nullptr;
        }
    }

private:
    std::unordered_map<void*, GLESv1Dispatch*> mGLESv1Dispatches;
    std::unordered_map<void*, GLESv2Dispatch*> mGLESv2Dispatches;
    std::unordered_map<void*, EGLDispatch*> mEGLDispatches;
};

static LazyInstance<DispatchStore> sDispatchStore = LAZY_INSTANCE_INIT;

// static
const GLESv1Dispatch* LazyLoadedGLESv1Dispatch::get() {
    LazyLoadedGLESv1Dispatch* instance = sGLESv1Dispatch.ptr();
    if (instance->mValid) {
        return &s_gles1;
    } else {
        return nullptr;
    }
}

LazyLoadedGLESv1Dispatch::LazyLoadedGLESv1Dispatch() {
    LazyLoadedEGLDispatch::get();
    mValid = gles1_dispatch_init(&s_gles1);
}

// static
const GLESv2Dispatch* LazyLoadedGLESv2Dispatch::get() {
    LazyLoadedGLESv2Dispatch* instance = sGLESv2Dispatch.ptr();
    if (instance->mValid) {
        return &s_gles2;
    } else {
        return nullptr;
    }
}

LazyLoadedGLESv2Dispatch::LazyLoadedGLESv2Dispatch() {
    LazyLoadedEGLDispatch::get();
    mValid = gles2_dispatch_init(&s_gles2);
}

// static
const EGLDispatch* LazyLoadedEGLDispatch::get() {
    LazyLoadedEGLDispatch* instance = sEGLDispatch.ptr();
    if (instance->mValid) {
        return &s_egl;
    } else {
        return nullptr;
    }
}

// static
const DispatchTables* LazyLoadedEGLDispatch::getLibrariesFrom(
        const char* eglPath,
        const char* glesv1Path,
        const char* glesv2Path,
        void* pipeDispatch) {

    emugl::SharedLibrary* eglLib = (emugl::SharedLibrary*)loadWithPipeDispatch(eglPath, pipeDispatch);
    emugl::SharedLibrary* gles1Lib = (emugl::SharedLibrary*)loadWithPipeDispatch(glesv1Path, pipeDispatch);
    emugl::SharedLibrary* gles2Lib = (emugl::SharedLibrary*)loadWithPipeDispatch(glesv2Path, pipeDispatch);

    if (pipeDispatch) {
        typedef void (*glesRegister_t)(void*,void*,void*);
        glesRegister_t regf = (glesRegister_t)(eglLib->findSymbol("registerCustomGLESLibraries"));
        if (regf) {
        fprintf(stderr, "%s: registered custom gles libs\n", __func__);
        regf(gles1Lib, gles2Lib, pipeDispatch);
        }
    }

    EGLDispatch* egl = sDispatchStore->loadEGL(eglLib);
    GLESv1Dispatch* gles1 = sDispatchStore->loadGLESv1(gles1Lib, egl);
    GLESv2Dispatch* gles2 = sDispatchStore->loadGLESv2(gles2Lib, egl);

    fprintf(stderr, "%s: gles2 create shader @ %p\n", __func__, gles2->glCreateShader);

    if (!egl || !gles1 || !gles2) {
        fprintf(stderr, "%s: failed to load EGL/GLESv1/GLESv2: EGL %p GLESv1 %p GLESv2 %p\n",
                __func__, egl, gles1, gles2);
        return nullptr;
    }

    DispatchTables* table = new DispatchTables;
    memcpy(&table->egl, egl, sizeof(EGLDispatch));
    memcpy(&table->gles1, gles1, sizeof(GLESv1Dispatch));
    memcpy(&table->gles2, gles2, sizeof(GLESv2Dispatch));

    table->eglLib = eglLib;
    table->gles1Lib = gles1Lib;
    table->gles2Lib = gles2Lib;

    return table;
}

// static
void* LazyLoadedEGLDispatch::loadWithPipeDispatch(const char* libPath, void* pipeDispatch) {
    char error[256];

    auto lib = emugl::SharedLibrary::open(libPath, error, sizeof(error));
    if (!lib) {
        fprintf(stderr, "%s: Could not load %s [%s]\n", __FUNCTION__,
                libPath, error);
        return 0;
    }

    if (!pipeDispatch) return lib;

    typedef void (*register_t)(void*);
    register_t registerFunc = (register_t)lib->findSymbol("registerPipeDispatch");
    if (registerFunc) {
        fprintf(stderr, "%s: establish pipe dispatch for %s\n", __func__, libPath);
        registerFunc(pipeDispatch);
    }

    return lib;
}

LazyLoadedEGLDispatch::LazyLoadedEGLDispatch() { mValid = init_egl_dispatch(); }
