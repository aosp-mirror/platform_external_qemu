// Copyright (C) 2015 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "FbConfig.h"

#include "emugl/common/feature_control.h"
#include "host-common/opengl/emugl_config.h"
#include "host-common/feature_control.h"
#include "host-common/misc.h"
#include "host-common/opengl/misc.h"
#include "FrameBuffer.h"
#include "OpenGLESDispatch/EGLDispatch.h"

#include <stdio.h>
#include <string.h>

namespace {

#define E(...)  fprintf(stderr, __VA_ARGS__)

#ifndef EGL_PRESERVED_RESOURCES
#define EGL_PRESERVED_RESOURCES 0x3030
#endif

const GLuint kConfigAttributes[] = {
    EGL_DEPTH_SIZE,     // must be first - see getDepthSize()
    EGL_STENCIL_SIZE,   // must be second - see getStencilSize()
    EGL_RENDERABLE_TYPE,// must be third - see getRenderableType()
    EGL_SURFACE_TYPE,   // must be fourth - see getSurfaceType()
    EGL_CONFIG_ID,      // must be fifth  - see chooseConfig()
    EGL_BUFFER_SIZE,
    EGL_ALPHA_SIZE,
    EGL_BLUE_SIZE,
    EGL_GREEN_SIZE,
    EGL_RED_SIZE,
    EGL_CONFIG_CAVEAT,
    EGL_LEVEL,
    EGL_MAX_PBUFFER_HEIGHT,
    EGL_MAX_PBUFFER_PIXELS,
    EGL_MAX_PBUFFER_WIDTH,
    EGL_NATIVE_RENDERABLE,
    EGL_NATIVE_VISUAL_ID,
    EGL_NATIVE_VISUAL_TYPE,
    EGL_PRESERVED_RESOURCES,
    EGL_SAMPLES,
    EGL_SAMPLE_BUFFERS,
    EGL_TRANSPARENT_TYPE,
    EGL_TRANSPARENT_BLUE_VALUE,
    EGL_TRANSPARENT_GREEN_VALUE,
    EGL_TRANSPARENT_RED_VALUE,
    EGL_BIND_TO_TEXTURE_RGB,
    EGL_BIND_TO_TEXTURE_RGBA,
    EGL_MIN_SWAP_INTERVAL,
    EGL_MAX_SWAP_INTERVAL,
    EGL_LUMINANCE_SIZE,
    EGL_ALPHA_MASK_SIZE,
    EGL_COLOR_BUFFER_TYPE,
    //EGL_MATCH_NATIVE_PIXMAP,
    EGL_RECORDABLE_ANDROID,
    EGL_CONFORMANT
};

const size_t kConfigAttributesLen =
        sizeof(kConfigAttributes) / sizeof(kConfigAttributes[0]);

bool isCompatibleHostConfig(EGLConfig config, EGLDisplay display) {
    // Filter out configs which do not support pbuffers, since they
    // are used to implement window surfaces.
    EGLint surfaceType;
    s_egl.eglGetConfigAttrib(
            display, config, EGL_SURFACE_TYPE, &surfaceType);
    if (!(surfaceType & EGL_PBUFFER_BIT)) {
        return false;
    }

    // Filter out configs that do not support RGB pixel values.
    EGLint redSize = 0, greenSize = 0, blueSize = 0;
    s_egl.eglGetConfigAttrib(
            display, config,EGL_RED_SIZE, &redSize);
    s_egl.eglGetConfigAttrib(
            display, config, EGL_GREEN_SIZE, &greenSize);
    s_egl.eglGetConfigAttrib(
            display, config, EGL_BLUE_SIZE, &blueSize);
    if (!redSize || !greenSize || !blueSize) {
        return false;
    }

    return true;
}

}  // namespace

FbConfig::~FbConfig() {
    delete [] mAttribValues;
}

FbConfig::FbConfig(EGLConfig hostConfig, EGLDisplay hostDisplay) :
        mEglConfig(hostConfig), mAttribValues() {
    mAttribValues = new GLint[kConfigAttributesLen];
    for (size_t i = 0; i < kConfigAttributesLen; ++i) {
        mAttribValues[i] = 0;
        s_egl.eglGetConfigAttrib(hostDisplay,
                                 hostConfig,
                                 kConfigAttributes[i],
                                 &mAttribValues[i]);

        // This implementation supports guest window surfaces by wrapping
        // them around host Pbuffers, so always report it to the guest.
        if (kConfigAttributes[i] == EGL_SURFACE_TYPE) {
            mAttribValues[i] |= EGL_WINDOW_BIT;
        }

        // Don't report ES3 renderable type if we don't support it.
        if (kConfigAttributes[i] == EGL_RENDERABLE_TYPE) {
            if (!emugl::emugl_feature_is_enabled(
                        android::featurecontrol::GLESDynamicVersion) &&
                mAttribValues[i] & EGL_OPENGL_ES3_BIT) {
                mAttribValues[i] &= ~EGL_OPENGL_ES3_BIT;
            }
        }
    }
}

FbConfigList::FbConfigList(EGLDisplay display) : mDisplay(display) {
    if (display == EGL_NO_DISPLAY) {
        E("%s: Invalid display value %p (EGL_NO_DISPLAY)\n",
          __FUNCTION__, (void*)display);
        return;
    }

    EGLint numHostConfigs = 0;
    if (!s_egl.eglGetConfigs(display, NULL, 0, &numHostConfigs)) {
        E("%s: Could not get number of host EGL configs\n", __FUNCTION__);
        return;
    }
    EGLConfig* hostConfigs = new EGLConfig[numHostConfigs];
    s_egl.eglGetConfigs(display, hostConfigs, numHostConfigs, &numHostConfigs);

    mConfigs = new FbConfig*[numHostConfigs];
    for (EGLint i = 0;  i < numHostConfigs; ++i) {
        // Filter out configs that are not compatible with our implementation.
        if (!isCompatibleHostConfig(hostConfigs[i], display)) {
            continue;
        }
        mConfigs[mCount] = new FbConfig(hostConfigs[i], display);
        mCount++;
    }

    delete [] hostConfigs;
}

FbConfigList::~FbConfigList() {
    for (int n = 0; n < mCount; ++n) {
        delete mConfigs[n];
    }
    delete [] mConfigs;
}

int FbConfigList::chooseConfig(const EGLint* attribs,
                               EGLint* configs,
                               EGLint configsSize) const {
    EGLint numHostConfigs = 0;
    if (!s_egl.eglGetConfigs(mDisplay, NULL, 0, &numHostConfigs)) {
        E("%s: Could not get number of host EGL configs\n", __FUNCTION__);
        return 0;
    }

    EGLConfig* matchedConfigs = new EGLConfig[numHostConfigs];

    // If EGL_SURFACE_TYPE appears in |attribs|, the value passed to
    // eglChooseConfig should be forced to EGL_PBUFFER_BIT because that's
    // what it used by the current implementation, exclusively. This forces
    // the rewrite of |attribs| into a new array.
    bool hasSurfaceType = false;
    bool wantSwapPreserved = false;
    int surfaceTypeIdx = 0;
    int numAttribs = 0;
    while (attribs[numAttribs] != EGL_NONE) {
        if (attribs[numAttribs] == EGL_SURFACE_TYPE) {
            hasSurfaceType = true;
            surfaceTypeIdx = numAttribs;
            if (attribs[numAttribs+1] & EGL_SWAP_BEHAVIOR_PRESERVED_BIT) {
                wantSwapPreserved = true;
            }
        }

        // Reject config if guest asked for ES3 and we don't have it.
        if (attribs[numAttribs] == EGL_RENDERABLE_TYPE) {
            if (attribs[numAttribs + 1] != EGL_DONT_CARE &&
                attribs[numAttribs + 1] & EGL_OPENGL_ES3_BIT_KHR &&
                (!emugl::emugl_feature_is_enabled(
                         android::featurecontrol::GLESDynamicVersion) ||
                 FrameBuffer::getMaxGLESVersion() <
                         GLES_DISPATCH_MAX_VERSION_3_0)) {
                return 0;
            }
        }
        numAttribs += 2;
    }

    std::vector<EGLint> newAttribs(numAttribs);
    memcpy(&newAttribs[0], attribs, numAttribs * sizeof(EGLint));

    int apiLevel;
    emugl::getAvdInfo(NULL, &apiLevel);

    if (!hasSurfaceType) {
        newAttribs.push_back(EGL_SURFACE_TYPE);
        newAttribs.push_back(0);
    } else if (wantSwapPreserved && apiLevel <= 19) {
        newAttribs[surfaceTypeIdx + 1] &= ~(EGLint)EGL_SWAP_BEHAVIOR_PRESERVED_BIT;
    }
    if (emugl::getRenderer() == SELECTED_RENDERER_SWIFTSHADER ||
        emugl::getRenderer() == SELECTED_RENDERER_SWIFTSHADER_INDIRECT ||
        emugl::getRenderer() == SELECTED_RENDERER_ANGLE ||
        emugl::getRenderer() == SELECTED_RENDERER_ANGLE_INDIRECT) {
        newAttribs.push_back(EGL_CONFIG_CAVEAT);
        newAttribs.push_back(EGL_DONT_CARE);
    }

    newAttribs.push_back(EGL_NONE);

    if (s_egl.eglChooseConfig(mDisplay,
                              &newAttribs[0],
                              matchedConfigs,
                              numHostConfigs,
                              &numHostConfigs) == EGL_FALSE) {
        delete [] matchedConfigs;
        return -s_egl.eglGetError();
    }

    int result = 0;
    for (int n = 0; n < numHostConfigs; ++n) {
        // Don't count or write more than |configsSize| items if |configs|
        // is not NULL.
        if (configs && configsSize > 0 && result >= configsSize) {
            break;
        }
        // Skip incompatible host configs.
        if (!isCompatibleHostConfig(matchedConfigs[n], mDisplay)) {
            continue;
        }
        // Find the FbConfig with the same EGL_CONFIG_ID
        EGLint hostConfigId;
        s_egl.eglGetConfigAttrib(
                mDisplay, matchedConfigs[n], EGL_CONFIG_ID, &hostConfigId);
        for (int k = 0; k < mCount; ++k) {
            int guestConfigId = mConfigs[k]->getConfigId();
            if (guestConfigId == hostConfigId) {
                // There is a match. Write it to |configs| if it is not NULL.
                if (configs && result < configsSize) {
                    configs[result] = (uint32_t)k;
                }
                result ++;
                break;
            }
        }
    }

    delete [] matchedConfigs;

    return result;
}


void FbConfigList::getPackInfo(EGLint* numConfigs,
                               EGLint* numAttributes) const {
    if (numConfigs) {
        *numConfigs = mCount;
    }
    if (numAttributes) {
        *numAttributes = static_cast<EGLint>(kConfigAttributesLen);
    }
}

EGLint FbConfigList::packConfigs(GLuint bufferByteSize, GLuint* buffer) const {
    GLuint numAttribs = static_cast<GLuint>(kConfigAttributesLen);
    GLuint kGLuintSize = static_cast<GLuint>(sizeof(GLuint));
    GLuint neededByteSize = (mCount + 1) * numAttribs * kGLuintSize;
    if (!buffer || bufferByteSize < neededByteSize) {
        return -neededByteSize;
    }
    // Write to the buffer the config attribute ids, followed for each one
    // of the configs, their values.
    memcpy(buffer, kConfigAttributes, kConfigAttributesLen * kGLuintSize);

    for (int i = 0; i < mCount; ++i) {
        memcpy(buffer + (i + 1) * kConfigAttributesLen,
               mConfigs[i]->mAttribValues,
               kConfigAttributesLen * kGLuintSize);
    }
    return mCount;
}
