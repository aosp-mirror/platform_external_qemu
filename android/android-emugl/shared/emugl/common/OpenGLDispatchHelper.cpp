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

#include "emugl/common/OpenGLDispatchHelper.h"

#include "android/base/memory/LazyInstance.h"

// Must be declared outside of MyGLESv2Dispatch scope due to the use of
// sizeof(T) within the template definition.
android::base::LazyInstance<MyGLESv2Dispatch> sGLESv2Dispatch =
        LAZY_INSTANCE_INIT;

// static
const GLESv2Dispatch* MyGLESv2Dispatch::get() {
    MyGLESv2Dispatch* instance = sGLESv2Dispatch.ptr();
    if (instance->mValid) {
        return &instance->mDispatch;
    } else {
        return nullptr;
    }
}

android::base::LazyInstance<MyEGLDispatch> sEGLDispatch = LAZY_INSTANCE_INIT;

// static
const EGLDispatch* MyEGLDispatch::get() {
    MyEGLDispatch* instance = sEGLDispatch.ptr();
    if (instance->mValid) {
        return &s_egl;
    } else {
        return nullptr;
    }
}
