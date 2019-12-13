// Copyright (C) 2019 The Android Open Source Project
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

#include "android/emulation/CudaVideoLibraryLoader.h"

#include "android/utils/dll.h"

namespace android {
namespace emulation {

bool CudaVideoLibraryLoader::openLibrary() {
    char* error = NULL;
    const char *CUDALIB = "libcuda";
    // 1. load libcuda first
    ADynamicLibrary* mCudaLib = adynamicLibrary_open(CUDALIB, &error);
    // 2. get all the cuda funcs

    const char *CODECLIB = "libnvcuvid";
    ADynamicLibrary* mCodecLib = adynamicLibrary_open(CODECLIB, &error);
    // 3. get all the codec funcs
}

}  // namespace emulation
}  // namespace android
