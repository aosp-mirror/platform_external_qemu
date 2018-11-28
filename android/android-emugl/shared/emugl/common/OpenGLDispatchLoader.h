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

#pragma once

#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv1Dispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

namespace emugl {

// Helper classes to hold global EGLDispatch, GLESv1Dispatch and GLESv2Dispatch
// objects, initialized lazily in a thread-safe way. The instances are leaked on
// program exit.

struct LazyLoadedGLESv1Dispatch {
    // Return pointer to global GLESv1Dispatch instance, or nullptr if there
    // was an error when trying to initialize/load the library.
    static const GLESv1Dispatch* get();

    LazyLoadedGLESv1Dispatch();

private:
    bool mValid;
};

struct LazyLoadedGLESv2Dispatch {
    // Return pointer to global GLESv2Dispatch instance, or nullptr if there
    // was an error when trying to initialize/load the library.
    static const GLESv2Dispatch* get();

    LazyLoadedGLESv2Dispatch();

private:
    bool mValid;
};

// Note that the dispatch table is provided by libOpenGLESDispatch as the
// 's_egl' global variable.
struct LazyLoadedEGLDispatch {
    // Return pointer to EGLDispatch table, or nullptr if there was
    // an error when trying to initialize/load the library.
    static const EGLDispatch* get();

    LazyLoadedEGLDispatch();

private:
    bool mValid;
};

}  // namespace emugl