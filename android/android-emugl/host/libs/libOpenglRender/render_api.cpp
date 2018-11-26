/*
* Copyright (C) 2011-2015 The Android Open Source Project
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
#include "OpenglRender/render_api.h"

#include "emugl/common/OpenGLDispatchLoader.h"
#include "ErrorLog.h"
#include "OpenGLESDispatch/DispatchTables.h"
#include "RenderLibImpl.h"

#include <memory>

RENDER_APICALL emugl::RenderLibPtr RENDER_APIENTRY initLibrary() {
    //
    // Load EGL Plugin
    //
    if (!emugl::LazyLoadedEGLDispatch::get()) {
        // Failed to load EGL
        printf("Failed to init_egl_dispatch\n");
        return nullptr;
    }

    //
    // Load GLES Plugin
    //
    if (!emugl::LazyLoadedGLESv1Dispatch::get()) {
        // Failed to load GLES
        ERR("Failed to gles1_dispatch_init\n");
        return nullptr;
    }

    /* failure to init the GLES2 dispatch table is not fatal */
    if (!emugl::LazyLoadedGLESv2Dispatch::get()) {
        ERR("Failed to gles2_dispatch_init\n");
        return nullptr;
    }

    return emugl::RenderLibPtr(new emugl::RenderLibImpl());
}
