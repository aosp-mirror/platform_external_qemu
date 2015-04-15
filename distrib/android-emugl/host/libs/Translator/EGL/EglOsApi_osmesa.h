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

#ifndef EGL_OS_API_OSMESA_H
#define EGL_OS_API_OSMESA_H

#include "EglOsApi.h"

namespace EglOS {

// Retrieve an instance of the EglOS::Engine interface class that is backed
// by the Offscreen Mesa (OSMesa) library, which is a pure-software renderer
// that doesn't support displaying native host windows.
//
// As such, createWindowSurface() and createPixmapSurface() will always
// return NULL.
Engine* getOSMesaEngineInstance();

}  // namespace EglOS

#endif  // EGL_OS_API_OSMESA_H
