/*
* Copyright (C) 2016 The Android Open Source Project
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

#include "GLESVersionDetector.h"

#include "DispatchTables.h"

#include "emugl/common/feature_control.h"

#include <algorithm>

GLESDispatchMaxVersion calcMaxVersionFromDispatch() {
    // Detect the proper OpenGL ES version to advertise to the system.
    //
    // If the underlying glesv2 library also is on top of system OpenGL,
    // we assume existence of a function called "gl_dispatch_get_max_version"
    // which matches the return type of gles2_dispatch_get_max_version,
    // and allows us to scale back if the underlying host machine
    // doesn't actually suppport a particular GLES version.
    if (!emugl_feature_is_enabled(android::featurecontrol::GLESDynamicVersion))
        return GLES_DISPATCH_MAX_VERSION_2;

    GLESDispatchMaxVersion underlying_gles2_lib_max =
        gles2_dispatch_get_max_version();
    if (s_gles2.gl_dispatch_get_max_version) {
        GLESDispatchMaxVersion underlying_gl_lib_max =
            (GLESDispatchMaxVersion)s_gles2.gl_dispatch_get_max_version();
        return std::min(underlying_gl_lib_max, underlying_gles2_lib_max);
    } else {
        return underlying_gles2_lib_max;
    }

    return GLES_DISPATCH_MAX_VERSION_2;
}
