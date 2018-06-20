// Copyright (C) 2018 The Android Open Source Project
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

#include "SearchPathsSetup.h"

#include "android/base/files/PathUtils.h"

#include "emugl/common/OpenGLDispatchLoader.h"
#include "emugl/common/shared_library.h"

#include <string>

using android::base::PathUtils;

namespace emugl {

static bool searchPathsSetup = false;

void setupStandaloneLibrarySearchPaths() {
#ifdef _WIN32
    if (searchPathsSetup) return;

    std::string lib64Path("lib64");
    std::string lib64SwiftshaderPath = PathUtils::join(lib64Path, "gles_swiftshader");
    SharedLibrary::addLibrarySearchPath(lib64Path.c_str());
    SharedLibrary::addLibrarySearchPath(lib64SwiftshaderPath.c_str());

    const auto egl = LazyLoadedEGLDispatch::get();
    egl->eglAddLibrarySearchPathANDROID(lib64Path.c_str());
    egl->eglAddLibrarySearchPathANDROID(lib64SwiftshaderPath.c_str());
    searchPathsSetup = true;
#endif
}

} // namespace emugl
