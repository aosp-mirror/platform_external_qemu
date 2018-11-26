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

#include "android/base/memory/LazyInstance.h"

#include "OpenGLESDispatch/DispatchTables.h"

GLESv1Dispatch s_gles1;
GLESv2Dispatch s_gles2;

using android::base::LazyInstance;
using emugl::LazyLoadedGLESv1Dispatch;
using emugl::LazyLoadedGLESv2Dispatch;
using emugl::LazyLoadedEGLDispatch;

// Must be declared outside of LazyLoaded*Dispatch scope due to the use of
// sizeof(T) within the template definition.
LazyInstance<LazyLoadedGLESv1Dispatch> sGLESv1Dispatch = LAZY_INSTANCE_INIT;
LazyInstance<LazyLoadedGLESv2Dispatch> sGLESv2Dispatch = LAZY_INSTANCE_INIT;
LazyInstance<LazyLoadedEGLDispatch> sEGLDispatch = LAZY_INSTANCE_INIT;

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

LazyLoadedEGLDispatch::LazyLoadedEGLDispatch() { mValid = init_egl_dispatch(); }
