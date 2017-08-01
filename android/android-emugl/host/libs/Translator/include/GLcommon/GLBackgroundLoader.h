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
#pragma once

#include "emugl/common/thread.h"

#include "GLcommon/TranslatorIfaces.h"

#include <EGL/egl.h>

class GLBackgroundLoader : public emugl::Thread {
public:
    GLBackgroundLoader(const EGLiface* eglIface, SaveableTextureMap* textureMap) :
        m_eglIface(eglIface),
        m_textureMap(textureMap) { }

    intptr_t main();

    const EGLiface* m_eglIface = nullptr;
    EGLDisplay m_dpy = nullptr;
    EGLConfig m_config = nullptr;
    EGLSurface m_surf = nullptr;
    EGLContext m_context = nullptr;

    SaveableTextureMap* m_textureMap = nullptr;
};
