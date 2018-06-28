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

    EGLDispatch* loadEGL(const char* libPath, emugl::SharedLibrary** outLibrary) {

        if (auto table = android::base::findOrDefault(mEGLDispatches,
                                                      libPath,
                                                      nullptr)) {
            return table;
        }

        EGLDispatch dispatch;
        if (init_egl_dispatch_from(libPath, &dispatch, outLibrary)) {
            EGLDispatch* allocatedDispatch = new EGLDispatch;
            mEGLDispatches[libPath] = allocatedDispatch;
            mEGLLibs[libPath] = *outLibrary;
            memcpy(allocatedDispatch, &dispatch, sizeof(EGLDispatch));
            return allocatedDispatch;
        } else {
            mEGLDispatches[libPath] = nullptr;
            return nullptr;
        }
    }

    GLESv1Dispatch* loadGLESv1(const char* libPath, EGLDispatch* egl_dispatch, emugl::SharedLibrary** outLibrary) {
        if (auto table = android::base::findOrDefault(mGLESv1Dispatches,
                                                    libPath,
                                                    nullptr)) {
            return table;
        }

        GLESv1Dispatch dispatch;
        if (gles1_dispatch_init_from(libPath, egl_dispatch, &dispatch, outLibrary)) {
            GLESv1Dispatch* allocatedDispatch = new GLESv1Dispatch;
            mGLESv1Dispatches[libPath] = allocatedDispatch;
            mGLESv1Libs[libPath] = *outLibrary;
            memcpy(allocatedDispatch, &dispatch, sizeof(GLESv1Dispatch));
            return allocatedDispatch;
        } else {
            mGLESv1Dispatches[libPath] = nullptr;
            return nullptr;
        }
    }

    GLESv2Dispatch* loadGLESv2(const char* libPath, EGLDispatch* egl_dispatch, emugl::SharedLibrary** outLibrary) {
        if (auto table = android::base::findOrDefault(mGLESv2Dispatches,
                                                    libPath,
                                                    nullptr)) {
            return table;
        }

        GLESv2Dispatch dispatch;
        if (gles2_dispatch_init_from(libPath, egl_dispatch, &dispatch, outLibrary)) {
            GLESv2Dispatch* allocatedDispatch = new GLESv2Dispatch;
            mGLESv2Dispatches[libPath] = allocatedDispatch;
            mGLESv2Libs[libPath] = *outLibrary;
            memcpy(allocatedDispatch, &dispatch, sizeof(GLESv2Dispatch));
            return allocatedDispatch;
        } else {
            mGLESv2Dispatches[libPath] = nullptr;
            return nullptr;
        }
    }

private:
    std::unordered_map<std::string, emugl::SharedLibrary*> mGLESv1Libs;
    std::unordered_map<std::string, emugl::SharedLibrary*> mGLESv2Libs;
    std::unordered_map<std::string, emugl::SharedLibrary*> mEGLLibs;

    std::unordered_map<std::string, GLESv1Dispatch*> mGLESv1Dispatches;
    std::unordered_map<std::string, GLESv2Dispatch*> mGLESv2Dispatches;
    std::unordered_map<std::string, EGLDispatch*> mEGLDispatches;
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
        const char* glesv2Path) {

    emugl::SharedLibrary* eglLib;
    emugl::SharedLibrary* gles1Lib;
    emugl::SharedLibrary* gles2Lib;
    EGLDispatch* egl = sDispatchStore->loadEGL(eglPath, &eglLib);
    GLESv1Dispatch* gles1 = sDispatchStore->loadGLESv1(glesv1Path, egl, &gles1Lib);
    GLESv2Dispatch* gles2 = sDispatchStore->loadGLESv2(glesv2Path, egl, &gles2Lib);

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

LazyLoadedEGLDispatch::LazyLoadedEGLDispatch() { mValid = init_egl_dispatch(); }
