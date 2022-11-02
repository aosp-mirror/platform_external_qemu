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
#ifndef EGL_DISPLAY_H
#define EGL_DISPLAY_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "emugl/common/mutex.h"
#include "emugl/common/smart_ptr.h"

#include "aemu/base/files/Stream.h"
#include "EglConfig.h"
#include "EglContext.h"
#include "EglOsApi.h"
#include "EglSurface.h"
#include "EglWindowSurface.h"
#include "GLcommon/ObjectNameSpace.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

typedef std::vector<std::unique_ptr<EglConfig>> ConfigsList;
typedef std::unordered_map<unsigned int, ContextPtr>  ContextsHndlMap;
typedef std::unordered_map<unsigned int, SurfacePtr>  SurfacesHndlMap;


typedef std::unordered_set<EglConfig> ConfigSet;

class EglDisplay {
public:
    // Create new EglDisplay instance from a given native display |dpy|,
    // with matching internal display |idpy|. If |isDefault| is true,
    // this will be considered the default diplay.
    EglDisplay(EGLNativeDisplayType dpy, EglOS::Display* idpy);

    // Return the EglOs Engine display handle for this EglDisplay.
    EGLNativeDisplayType getEglOsEngineDisplay() const { return m_dpy; }

    // Return the native internal display handle for this EglDisplay.
    EglOS::Display* nativeType() const { return m_idpy; }

    // Return the number of known configurations for this EglDisplay.
    int nConfigs() const { return m_configs.size(); }

    // Returns the max supported GLES version
    EglOS::GlesVersion getMaxGlesVersion() const {
        return nativeType()->getMaxGlesVersion();
    }

    const char* getExtensionString() {
        return nativeType()->getExtensionString();
    }

    EGLImage createNativeImage(
            EGLDisplay dpy,
            EGLContext ctx,
            EGLenum target,
            EGLClientBuffer buffer,
            const EGLint *attrib_list) {
        return nativeType()->createImage(dpy, ctx, target, buffer, attrib_list);
    }

    EGLBoolean destroyNativeImage(
            EGLDisplay dpy,
            EGLImage image) {
        return nativeType()->destroyImage(dpy, image);
    }

    EGLDisplay getHostDriverDisplay() {
        return nativeType()->getNative();
    }

    void* getNativeContext(EGLContext ctx) const {
        auto dispContext = getContext(ctx);
        if (!dispContext) return nullptr;
        return dispContext->nativeType()->getNative();
    }

    // Write up to |config_size| EGLConfig values into the |configs| array.
    // Return the number if values written.
    int getConfigs(EGLConfig* configs,int config_size) const;

    // Select all display configurations that match at least the values
    // in |dummy|. If |configs| is NULL, this returns the number of all
    // matching configs. Otherwise, this writes into |configs| up to
    // |config_size| matching EGLConfig values, and returns their number.
    int chooseConfigs(const EglConfig& dummy,
                      EGLConfig* configs,
                      int config_size) const;

    // Return the EglConfig value that matches a given EGLConfig |conf|.
    EglConfig* getConfig(EGLConfig conf) const;

    // Return the EglConfig value that matches a given EGLConfig with
    // EGL_CONFIG_ID value |id|.
    EglConfig* getConfig(EGLint id) const;

    EglConfig* getDefaultConfig() const;

    EGLSurface addSurface(SurfacePtr s );
    SurfacePtr getSurface(EGLSurface surface) const;
    bool removeSurface(EGLSurface s);
    EGLContext addContext(ContextPtr ctx );
    ContextPtr getContext(EGLContext ctx) const;
    void* getLowLevelContext(EGLContext ctx) const;
    bool removeContext(EGLContext ctx);
    bool removeContext(ContextPtr ctx);
    ObjectNameManager* getManager(GLESVersion ver) const { return m_manager[ver];}

    ~EglDisplay();
    void initialize(int renderableType);
    void terminate();
    bool isInitialize();

    ImagePtr getImage(EGLImageKHR img,
        SaveableTexture::restorer_t restorer) const;
    EGLImageKHR addImageKHR(ImagePtr);
    bool destroyImageKHR(EGLImageKHR img);
    EglOS::Context* getGlobalSharedContext() const;
    GlobalNameSpace* getGlobalNameSpace() { return &m_globalNameSpace; }

    void onSaveAllImages(android::base::Stream* stream,
                         const android::snapshot::ITextureSaverPtr& textureSaver,
                         SaveableTexture::saver_t saver,
                         SaveableTexture::restorer_t restorer);
    void onLoadAllImages(android::base::Stream* stream,
                         const android::snapshot::ITextureLoaderPtr& textureLoader,
                         SaveableTexture::creator_t creator);
    void postLoadAllImages(android::base::Stream* stream);

private:
    static void addConfig(void* opaque, const EglOS::ConfigInfo* configInfo);

    int doChooseConfigs(const EglConfig& dummy,EGLConfig* configs,int config_size) const;
    EglConfig* addSimplePixelFormat(int red_size, int green_size, int blue_size, int alpha_size, int sample_per_pixel);
    void addReservedConfigs(void);
    void initConfigurations(int renderableType);

    EGLNativeDisplayType    m_dpy = {};
    EglOS::Display*         m_idpy = nullptr;
    bool                    m_initialized = false;
    bool                    m_configInitialized = false;
    ConfigsList             m_configs;
    ContextsHndlMap         m_contexts;
    SurfacesHndlMap         m_surfaces;
    GlobalNameSpace         m_globalNameSpace;
    ObjectNameManager*      m_manager[MAX_GLES_VERSION];
    mutable emugl::Mutex    m_lock;
    ImagesHndlMap           m_eglImages;
    unsigned int            m_nextEglImageId = 0;
    mutable emugl::SmartPtr<EglOS::Context> m_globalSharedContext;
    ConfigSet               m_uniqueConfigs;
};

#endif
