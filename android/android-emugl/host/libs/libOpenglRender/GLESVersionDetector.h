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
#pragma once

#include "OpenGLESDispatch/GLESv2Dispatch.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <string>
// Used to determine maximum supported GLES version.
GLESDispatchMaxVersion calcMaxVersionFromDispatch(EGLDisplay dpy);

// For determining whether or not to use core profile OpenGL.
bool shouldEnableCoreProfile();

std::string filterExtensionsBasedOnMaxVersion(GLESDispatchMaxVersion ver, const std::string& exts);

