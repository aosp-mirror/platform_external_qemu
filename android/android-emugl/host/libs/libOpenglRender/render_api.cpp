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

#include "ErrorLog.h"
#include "GLESVersionDetector.h"
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv1Dispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"
#include "RenderLibImpl.h"

#include "emugl/common/feature_control.h"
#include "emugl/common/misc.h"

#include <memory>

GLESv2Dispatch s_gles2;
GLESv1Dispatch s_gles1;

RENDER_APICALL emugl::RenderLibPtr RENDER_APIENTRY
initLibrary(render_api_feature_is_enabled_t feature_is_enabled_func) {
    //
    // Load EGL Plugin
    //
    if (!init_egl_dispatch()) {
        // Failed to load EGL
        printf("Failed to init_egl_dispatch\n");
        return nullptr;
    }

    //
    // Load GLES Plugin
    //
    if (!gles1_dispatch_init(&s_gles1)) {
        // Failed to load GLES
        ERR("Failed to gles1_dispatch_init\n");
        return nullptr;
    }

    /* failure to init the GLES2 dispatch table is not fatal */
    if (!gles2_dispatch_init(&s_gles2)) {
        ERR("Failed to gles2_dispatch_init\n");
        return nullptr;
    }

    emugl::RenderLibPtr res(new emugl::RenderLibImpl());
    res->setFeatureController((emugl_feature_is_enabled_t)feature_is_enabled_func);
    /* Run GLES version detector now that we have the dlls
     * and feature control function. This is needed to detect whether to use
     * core profile and it needs at least be done earlier than Framebuffer
     * initialization; here, it's done as early as possible.*/
    calcMaxVersionFromDispatch();

    return res;
}

RENDER_APICALL bool RENDER_APIENTRY
renderApiFeatureIsEnabled(uint32_t feature) {
    return emugl_feature_is_enabled((android::featurecontrol::Feature)feature);
}

RENDER_APICALL void RENDER_APIENTRY
renderApiGlesVersion(int* maj, int* min) {
    emugl::getGlesVersion(maj, min);
}
