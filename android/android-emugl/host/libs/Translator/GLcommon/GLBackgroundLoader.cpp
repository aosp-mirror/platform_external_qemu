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

#include "GLcommon/GLBackgroundLoader.h"

#include "GLcommon/GLEScontext.h"
#include "GLcommon/SaveableTexture.h"

#include <EGL/eglext.h>
#include <GLES2/gl2.h>

intptr_t GLBackgroundLoader::main() {
    /*
    // create the context
    m_dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglBindAPI(EGL_OPENGL_ES_API);

    static const GLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT, EGL_NONE };

    int numConfigs;
    if (!eglChooseConfig(m_dpy, configAttribs, &m_config, 1, &numConfigs) ||
        numConfigs == 0) {
        fprintf(stderr, "%s: could not find gles 2 config!\n", __func__);
        return NULL;
    }

    static const EGLint pbufAttribs[] = {
        EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    m_surf = eglCreatePbufferSurface(m_dpy, m_config, pbufAttribs);
    if (!m_surf) {
        fprintf(stderr, "%s: could not create surface\n", __func__);
        return NULL;
    }

    static const GLint gles2Attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,
        EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,
        EGL_NONE };

    m_context = eglCreateContext(m_dpy, m_config, EGL_NO_CONTEXT, gles2Attribs);

    if (!eglMakeCurrent(m_dpy, m_surf, m_surf, m_context)) {
        fprintf(stderr, "%s: eglMakeCurrent failed\n", __func__);
        return NULL;
    }
    */

    fprintf(stderr, "%s: start background loading %p\n", __func__,
            m_eglIface);
    if (!m_eglIface->createAndBindAuxiliaryContext()) {
        return 0;
    }

    for (auto it: *m_textureMap) {
        SaveableTexturePtr saveable = it.second;
        if (saveable) {
            saveable->touch();
        }
    }

    m_textureMap->clear();
    return 0;
}
