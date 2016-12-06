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

unsigned int EglContext::s_nextContextHndl = 0;

extern EglGlobalInfo* g_eglInfo; // defined in EglImp.cpp

bool EglContext::usingSurface(SurfacePtr surface) {
  return surface.get() == m_read.get() || surface.get() == m_draw.get();
}

EglContext::EglContext(EglDisplay *dpy,
                       EglOS::Context* context,
                       ContextPtr shared_context,
                       EglConfig* config,
                       GLEScontext* glesCtx,
                       GLESVersion ver,
                       ObjectNameManager* mngr) :
        m_dpy(dpy),
        m_native(context),
        m_config(config),
        m_glesContext(glesCtx),
        m_version(ver),
        m_mngr(mngr) {
    m_shareGroup = shared_context.get()?
                   mngr->attachShareGroup(context,shared_context->nativeType()):
                   mngr->createShareGroup(context);
    m_hndl = ++s_nextContextHndl;
}

EglContext::~EglContext()
{
    //
    // Delete the ShareGroup first to make sure context still exists.
    // If it's already removed, Mac driver crashes on null pointer access in
    // eglDelete...() calls.
    //
    if (m_mngr)
    {
        m_mngr->deleteShareGroup(m_native);
    }
    g_eglInfo->getIface(version())->setShareGroup(m_glesContext, {});
    m_shareGroup.reset();

    //
    // remove the context in the underlying OS layer
    //
    m_dpy->nativeType()->destroyContext(m_native);

    //
    // call the client-api to remove the GLES context
    // 
    g_eglInfo->getIface(version())->deleteGLESContext(m_glesContext);
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

bool EglContext::attachImage(unsigned int imageId,ImagePtr img){
   if(m_attachedImages.find(imageId) == m_attachedImages.end()){
       m_attachedImages[imageId] = img;
       return true;
   }
   return false;
}

void EglContext::detachImage(unsigned int imageId){
    m_attachedImages.erase(imageId);
}

