/*
* Copyright (C) 2017 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "EglOsApi.h"

#include "emugl/common/feature_control.h"
#include "emugl/common/lazy_instance.h"
#include "emugl/common/shared_library.h"

namespace EglOS {

#if defined(__APPLE__)
static const char kRenderApiLibName[] = "lib64OpenglRender.dylib";
#elif !defined(__x86_64__) && defined(_WIN32)
static const char kRenderApiLibName[] = "lib64OpenglRender.dll";
#elif defined(_WIN32)
static const char kRenderApiLibName[] = "libOpenglRender.dll";
#else
static const char kRenderApiLibName[] = "lib64OpenglRender.so";
#endif

static const char kFeatureIsEnabledFuncName[] = "renderApiFeatureIsEnabled";
static const char kGetGlesVersionFuncName[] = "renderApiGlesVersion";

typedef bool (*feature_enabled_t)(android::featurecontrol::Feature);
typedef void (*get_gles_version_t)(int*, int*);

struct EglOsRenderApi {
    emugl::SharedLibrary* renderApiLib = nullptr;
    feature_enabled_t featureIsEnabled = nullptr;
    get_gles_version_t getGlesVersion = nullptr;
};

emugl::LazyInstance<EglOsRenderApi> sRenderApi = LAZY_INSTANCE_INIT;

bool shouldEnableCoreProfile() {
    EglOsRenderApi* api = sRenderApi.ptr();

    if (!api->renderApiLib) {
        char error[256];
        api->renderApiLib =
            emugl::SharedLibrary::open(
                "lib64OpenglRender.dylib",
                error, sizeof(error));
        if (!api->renderApiLib) {
            fprintf(stderr, "%s: no render api lib!\n", __func__);
            return false;
        }
    }

    if (!api->featureIsEnabled) {
        api->featureIsEnabled =
            reinterpret_cast<feature_enabled_t>(
                api->renderApiLib->findSymbol("renderApiFeatureIsEnabled"));
        if (!api->featureIsEnabled) {
            fprintf(stderr, "%s: no feature api!\n", __func__);
            return false;
        }
    }

    if (!api->getGlesVersion) {
        api->getGlesVersion =
            reinterpret_cast<get_gles_version_t>(
                api->renderApiLib->findSymbol("renderApiGlesVersion"));
        if (!api->getGlesVersion) {
            fprintf(stderr, "%s: no glesversion api!\n", __func__);
            return false;
        }
    }

    int majorVersion = 2;
    int minorVersion = 0;
    api->getGlesVersion(&majorVersion, &minorVersion);

    return majorVersion > 2;
}

} // namespace EglOS

