/*
* Copyright (C) 2011 The Android Open Source Project
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
#include "EglGlobalInfo.h"

#include "ClientAPIExts.h"
#include "EglDisplay.h"
#include "EglOsApi.h"
#include "EglOsApi_osmesa.h"

#include "OpenglCodecCommon/ErrorLog.h"

#include "emugl/common/lazy_instance.h"

#include <stdlib.h>
#include <string.h>

namespace {

// Use a LazyInstance to ensure thread-safe initialization.
emugl::LazyInstance<EglGlobalInfo> sSingleton = LAZY_INSTANCE_INIT;

}  // namespace

// static
EglGlobalInfo* EglGlobalInfo::getInstance() {
    return sSingleton.ptr();
}

EglGlobalInfo::EglGlobalInfo() :
        m_displays(),
        m_engine(NULL),
        m_display(NULL),
        m_lock() {
    const char* env = ::getenv("ANDROID_EGL_ENGINE");
    if (env) {
        if (!strcmp(env, "osmesa")) {
            m_engine = EglOS::getOSMesaEngineInstance();
            if (!m_engine) {
                ERR("%s: Could not load OSMesa library!\n", __FUNCTION__);
            }
        } else {
            ERR("%s: Unknown ANDROID_EGL_ENGINE value '%s' ignored\n",
                __FUNCTION__, env);
        }
    }
    if (!m_engine) {
        m_engine = EglOS::Engine::getHostInstance();
    }
    m_display = m_engine->getDefaultDisplay();

    memset(m_gles_ifaces, 0, sizeof(m_gles_ifaces));
    memset(m_gles_extFuncs_inited, 0, sizeof(m_gles_extFuncs_inited));
}

EglGlobalInfo::~EglGlobalInfo() {
    for (size_t n = 0; n < m_displays.size(); ++n) {
        delete m_displays[n];
    }
}

EglDisplay* EglGlobalInfo::addDisplay(EGLNativeDisplayType dpy,
                                      EglOS::Display* idpy) {
    //search if it already exists.
    emugl::Mutex::AutoLock mutex(m_lock);
    for (size_t n = 0; n < m_displays.size(); ++n) {
        if (m_displays[n]->getNativeDisplay() == dpy) {
            return m_displays[n];
        }
    }

    if (!idpy) {
        return NULL;
    }
    EglDisplay* result = new EglDisplay(dpy, idpy);
    m_displays.push_back(result);
    return result;
}

bool  EglGlobalInfo::removeDisplay(EGLDisplay dpy) {
    emugl::Mutex::AutoLock mutex(m_lock);
    for (size_t n = 0; n < m_displays.size(); ++n) {
        if (m_displays[n] == static_cast<EglDisplay*>(dpy)) {
            delete m_displays[n];
            m_displays.remove(n);
            return true;
        }
    }
    return false;
}

EglDisplay* EglGlobalInfo::getDisplay(EGLNativeDisplayType dpy) const {
    emugl::Mutex::AutoLock mutex(m_lock);
    for (size_t n = 0; n < m_displays.size(); ++n) {
        if (m_displays[n]->getNativeDisplay() == dpy) {
            return m_displays[n];
        }
    }
    return NULL;
}

EglDisplay* EglGlobalInfo::getDisplay(EGLDisplay dpy) const {
    emugl::Mutex::AutoLock mutex(m_lock);
    for (size_t n = 0; n < m_displays.size(); ++n) {
        if (m_displays[n] == static_cast<EglDisplay*>(dpy)) {
            return m_displays[n];
        }
    }
    return NULL;
}

// static
EglOS::Display* EglGlobalInfo::generateInternalDisplay(
        EGLNativeDisplayType dpy) {
    return getInstance()->m_engine->getInternalDisplay(dpy);
}

void EglGlobalInfo::initClientExtFuncTable(GLESVersion ver) {
    emugl::Mutex::AutoLock mutex(m_lock);
    if (!m_gles_extFuncs_inited[ver]) {
        ClientAPIExts::initClientFuncs(m_gles_ifaces[ver], (int)ver - 1);
        m_gles_extFuncs_inited[ver] = true;
    }
}
