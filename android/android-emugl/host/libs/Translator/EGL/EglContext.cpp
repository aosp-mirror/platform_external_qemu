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
#include "EglContext.h"
#include "EglDisplay.h"
#include "EglGlobalInfo.h"
#include "EglOsApi.h"
#include "EglPbufferSurface.h"
#include "ThreadInfo.h"

unsigned int EglContext::s_nextContextHndl = 0;

extern EglGlobalInfo* g_eglInfo; // defined in EglImp.cpp

bool EglContext::usingSurface(SurfacePtr surface) {
  return surface.get() == m_read.get() || surface.get() == m_draw.get();
}

EglContext::EglContext(EglDisplay *dpy,
                       const ContextPtr& shared_context,
                       uint64_t shareGroupId,
                       EglConfig* config,
                       GLEScontext* glesCtx,
                       GLESVersion ver,
                       ObjectNameManager* mngr,
                       android::base::Stream* stream) :
        m_dpy(dpy),
        m_config(config),
        m_glesContext(glesCtx),
        m_version(ver),
        m_mngr(mngr)
{

    EglOS::Context* globalSharedContext = dpy->getGlobalSharedContext();
    m_native = dpy->nativeType()->createContext(
            m_config->nativeFormat(), globalSharedContext);
    if (stream) {
        EGLint configId = EGLint(stream->getBe32());
        m_config = dpy->getConfig(configId);
        shareGroupId = static_cast<uint64_t>(stream->getBe64());
    }
    if (shared_context.get()) {
        m_shareGroup = mngr->attachShareGroup(m_native,shared_context->nativeType());
        m_hndl = ++s_nextContextHndl;
    } else {
        if (m_native) {
            m_shareGroup = mngr->attachOrCreateShareGroup(m_native, shareGroupId);
            m_hndl = ++s_nextContextHndl;
        } else {
            m_hndl = 0;
        }
    }
}

EglContext::~EglContext()
{
    ThreadInfo* thread = getThreadInfo();
    // get the current context
    ContextPtr currentCtx = thread->eglContext;
    if (currentCtx && !m_dpy->getContext((EGLContext)SafePointerFromUInt(
                        currentCtx->getHndl()))) {
        currentCtx.reset();
    }
    SurfacePtr currentRead = currentCtx ? currentCtx->read() : nullptr;
    SurfacePtr currentDraw = currentCtx ? currentCtx->draw() : nullptr;
    // we need to make the context current before releasing GL resources.
    // create a dummy surface first
    EglPbufferSurface pbSurface(m_dpy, m_config);
    pbSurface.setAttrib(EGL_WIDTH, 1);
    pbSurface.setAttrib(EGL_HEIGHT, 1);
    EglOS::PbufferInfo pbInfo;
    pbSurface.getDim(&pbInfo.width, &pbInfo.height, &pbInfo.largest);
    pbSurface.getTexInfo(&pbInfo.target, &pbInfo.format);
    pbInfo.hasMipmap = false;
    EglOS::Surface* pb = m_dpy->nativeType()->createPbufferSurface(
            m_config->nativeFormat(), &pbInfo);
    assert(pb);
    if (pb) {
        const bool res = m_dpy->nativeType()->makeCurrent(pb, pb, m_native);
        assert(res);
        (void)res;
        pbSurface.setNativePbuffer(pb);
    }
    //
    // release GL resources. m_shareGroup, m_mngr and m_glesContext hold
    // smart pointers to share groups. We must clean them up when the context
    // is current.
    //
    g_eglInfo->getIface(version())->setShareGroup(m_glesContext, {});
    if (m_mngr) {
        m_mngr->deleteShareGroup(m_native);
    }
    m_shareGroup.reset();

    //
    // call the client-api to remove the GLES context
    //
    g_eglInfo->getIface(version())->deleteGLESContext(m_glesContext);
    //
    // restore the current context
    //
    if (currentCtx) {
        m_dpy->nativeType()->makeCurrent(currentRead->native(),
                                         currentDraw->native(),
                                         currentCtx->nativeType());
    } else {
        m_dpy->nativeType()->makeCurrent(nullptr, nullptr, nullptr);
    }

    //
    // remove the context in the underlying OS layer
    //
    m_dpy->nativeType()->destroyContext(m_native);
}

void EglContext::setSurfaces(SurfacePtr read,SurfacePtr draw)
{
    m_read = read;
    m_draw = draw;
}

bool EglContext::getAttrib(EGLint attrib,EGLint* value) {
    switch(attrib) {
    case EGL_CONFIG_ID:
        *value = m_config->id();
        break;
    default:
        return false;
    }
    return true;
}

void EglContext::onSave(android::base::Stream* stream) {
    // We save the information that
    // is needed to restore the contexts.
    // That means (1) context configurations (2) shared group IDs.

    // Save the config.
    // The current implementation is pretty hacky. It stores the config id.
    // It almost only works when snapshot saving and loading happens on the
    // same system with the same GPU driver and hardware.
    // TODO: make it more general
    stream->putBe32(getConfig()->id());
    // Save shared group ID
    stream->putBe64(m_shareGroup->getId());
}
