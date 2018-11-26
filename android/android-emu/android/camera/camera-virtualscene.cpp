/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "android/camera/camera-virtualscene.h"

#include "android/base/memory/LazyInstance.h"
#include "android/camera/camera-format-converters.h"
#include "android/virtualscene/VirtualSceneManager.h"
#include "emugl/common/OpenGLDispatchLoader.h"

#include <vector>

#define E(...) derror(__VA_ARGS__)
#define W(...) dwarning(__VA_ARGS__)
#define D(...) VERBOSE_PRINT(camera, __VA_ARGS__)
#define D_ACTIVE VERBOSE_CHECK(camera)

#define VIRTUALSCENE_PIXEL_FORMAT V4L2_PIX_FMT_RGB32

namespace android {
namespace virtualscene {

// Defines an RAII object to automatically unset the EGL context when the scope
// exits. Returned by VirtualSceneCameraDevice::makeEglCurrent.
class ScopedEglContext {
    DISALLOW_COPY_AND_ASSIGN(ScopedEglContext);

public:
    // Unset the EGL context on scope exit.
    // |eglDispatch| - If non-null, the EGLDispatch object will be used to call
    //                 eglMakeCurrent with EGL_NO_CONTEXT. If this is non-null,
    //                 |eglDisplay| must also be EGL_NO_DISPLAY.
    // |eglDisplay| - EGLDisplay handle, if |eglDispatch| is null, this must be
    //                set to EGL_NO_DISPLAY.
    ScopedEglContext(const EGLDispatch* eglDispatch,
                     const EGLDisplay eglDisplay)
        : mEglDispatch(eglDispatch), mEglDisplay(eglDisplay) {
        AASSERT((eglDispatch && eglDisplay != EGL_NO_DISPLAY) ||
                (!eglDispatch && eglDisplay == EGL_NO_DISPLAY));
    }

    ScopedEglContext(ScopedEglContext&& other)
        : mEglDispatch(other.mEglDispatch), mEglDisplay(other.mEglDisplay) {
        other.mEglDispatch = nullptr;
        other.mEglDisplay = nullptr;
    }

    ~ScopedEglContext() {
        if (mEglDispatch) {
            const EGLBoolean eglResult = mEglDispatch->eglMakeCurrent(
                    mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                    EGL_NO_CONTEXT);
            if (eglResult == EGL_FALSE) {
                W("%s: Could not unset eglMakeCurrent error %d", __FUNCTION__,
                  mEglDispatch->eglGetError());
            }
        }
    }

    bool isValid() const { return mEglDispatch; }

private:
    const EGLDispatch* mEglDispatch;
    EGLDisplay mEglDisplay;
};

/*******************************************************************************
 *                     VirtualSceneCameraDevice routines
 ******************************************************************************/

/*
 * Describes a connection to an actual camera device.
 */
class VirtualSceneCameraDevice {
    DISALLOW_COPY_AND_ASSIGN(VirtualSceneCameraDevice);

public:
    VirtualSceneCameraDevice();
    ~VirtualSceneCameraDevice();

    CameraDevice* getCameraDevice() { return &mHeader; }

    int startCapturing(uint32_t pixelFormat, int frameWidth, int frameHeight);
    void stopCapturing();

    int readFrame(ClientFrame* resultFrame,
                  float rScale,
                  float gScale,
                  float bScale,
                  float expComp);

private:
    // Initialize EGL, returns false on failure.
    bool initializeEgl();

    // Set the EGL context active on the current thread. This call modifies the
    // thread-local EGL state to make the VirtualSceneCameraDevice's EGL context
    // active. This may be called on any thread, but callers should be resilient
    // to having their EGL state reset when the ScopedEglContext RAII object
    // goes out of scope.
    ScopedEglContext makeEglCurrent();

    // Common camera header.
    CameraDevice mHeader;

    std::vector<uint8_t> mFramebufferData;

    int mFramebufferWidth = 0;
    int mFramebufferHeight = 0;

    // Dispatch tables for EGL and GLESv2 APIs. Note that these will be nullptr
    // if there was a problem when loading the host libraries.
    const EGLDispatch* mEglDispatch = nullptr;
    const GLESv2Dispatch* mGles2 = nullptr;

    EGLDisplay mEglDisplay = EGL_NO_DISPLAY;
    EGLContext mEglContext = EGL_NO_CONTEXT;
    EGLSurface mEglSurface = EGL_NO_SURFACE;
    bool mEglInitialized = false;
};

VirtualSceneCameraDevice::VirtualSceneCameraDevice() {
    mHeader.opaque = this;
}

VirtualSceneCameraDevice::~VirtualSceneCameraDevice() {
    stopCapturing();
}

int VirtualSceneCameraDevice::startCapturing(uint32_t pixelFormat,
                                             int frameWidth,
                                             int frameHeight) {
    D("%s: Start capturing at %d x %d", __FUNCTION__, frameWidth, frameHeight);

    mFramebufferData.resize(4 * frameWidth * frameHeight);

    mFramebufferWidth = frameWidth;
    mFramebufferHeight = frameHeight;

    bool succeeded = false;
    if (initializeEgl()) {
        auto context = makeEglCurrent();
        if (context.isValid()) {
            if (VirtualSceneManager::initialize(mGles2, frameWidth,
                                                frameHeight)) {
                succeeded = true;
            } else {
                E("%s: VirtualSceneManager initialize failed", __FUNCTION__);
            }
        }
    }

    if (!succeeded) {
        stopCapturing();
        return -1;
    }

    return 0;
}

// Resets camera device after capturing.
// Since new capture request may require different frame dimensions we must
// reset camera device by reopening its handle. Otherwise attempts to set up new
// frame properties (different from the previous one) may fail.
void VirtualSceneCameraDevice::stopCapturing() {
    if (mEglInitialized) {
        // Only call makeEglCurrent if egl is initialized, to avoid an infinite
        // loop when eglMakeCurrent fails.
        auto context = makeEglCurrent();
        if (context.isValid()) {
            VirtualSceneManager::uninitialize();
        }
    }

    if (mEglDispatch && mEglDisplay != EGL_NO_DISPLAY) {
        mEglDispatch->eglDestroySurface(mEglDisplay, mEglSurface);
        mEglDispatch->eglDestroyContext(mEglDisplay, mEglContext);
        // Don't eglTerminate the display, we don't own the instance.
        mEglDisplay = EGL_NO_DISPLAY;
    }

    mFramebufferData.clear();
    mFramebufferData.shrink_to_fit();
    mFramebufferWidth = 0;
    mFramebufferHeight = 0;

    mEglInitialized = false;
}

int VirtualSceneCameraDevice::readFrame(ClientFrame* resultFrame,
                                        float rScale,
                                        float gScale,
                                        float bScale,
                                        float expComp) {
    auto context = makeEglCurrent();
    if (!context.isValid()) {
        return -1;
    }

    resultFrame->frame_time = VirtualSceneManager::render();
    mEglDispatch->eglSwapBuffers(mEglDisplay, mEglSurface);
    mGles2->glReadPixels(0, 0, mFramebufferWidth, mFramebufferHeight, GL_RGBA,
                         GL_UNSIGNED_BYTE, mFramebufferData.data());

    // Convert frame to the receiving buffers.
    return convert_frame(mFramebufferData.data(), VIRTUALSCENE_PIXEL_FORMAT,
                         mFramebufferData.size(), mFramebufferWidth,
                         mFramebufferHeight, resultFrame, rScale, gScale,
                         bScale, expComp);
}

bool VirtualSceneCameraDevice::initializeEgl() {
    mEglDispatch = emugl::LazyLoadedEGLDispatch::get();
    mGles2 = emugl::LazyLoadedGLESv2Dispatch::get();
    mEglDisplay = mEglDispatch->eglGetDisplay(EGL_DEFAULT_DISPLAY);

    if (mEglDisplay == EGL_NO_DISPLAY) {
        E("%s: eglGetDisplay failed, error %d", __FUNCTION__,
          mEglDispatch->eglGetError());
        return false;
    }

    EGLint eglMaj = 0;
    EGLint eglMin = 0;

    // Try to initialize EGL display.
    // Initializing an already-initialized display is OK.
    if (mEglDispatch->eglInitialize(mEglDisplay, &eglMaj, &eglMin) ==
        EGL_FALSE) {
        E("%s: eglInitialize failed, error %d", __FUNCTION__,
          mEglDispatch->eglGetError());
        return false;
    }

    // Get an EGL config.
    const EGLint attribs[] = {EGL_SURFACE_TYPE,
                              EGL_PBUFFER_BIT,
                              EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_ES_BIT,
                              EGL_BLUE_SIZE,
                              8,
                              EGL_GREEN_SIZE,
                              8,
                              EGL_RED_SIZE,
                              8,
                              EGL_DEPTH_SIZE,
                              16,
                              EGL_NONE};
    EGLint numConfig = 0;
    EGLConfig eglConfig;
    EGLBoolean chooseResult = mEglDispatch->eglChooseConfig(
            mEglDisplay, attribs, &eglConfig, 1, &numConfig);
    if (chooseResult == EGL_FALSE || numConfig < 1) {
        E("%s: eglChooseConfig failed, error %d", __FUNCTION__,
          mEglDispatch->eglGetError());
        return false;
    }

    // Create a context.
    const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    mEglContext = mEglDispatch->eglCreateContext(
            mEglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (mEglContext == EGL_NO_CONTEXT) {
        E("%s: eglCreateContext failed, error %d", __FUNCTION__,
          mEglDispatch->eglGetError());
        return false;
    }

    // Finally, create a window surface associated with this widget.
    const EGLint pbufferAttribs[] = {EGL_WIDTH, mFramebufferWidth,
                                     EGL_HEIGHT, mFramebufferHeight,
                                     EGL_NONE};
    mEglSurface = mEglDispatch->eglCreatePbufferSurface(mEglDisplay, eglConfig,
                                                        pbufferAttribs);
    if (mEglSurface == EGL_NO_SURFACE) {
        E("%s: eglCreatePbufferSurface failed, error %d", __FUNCTION__,
          mEglDispatch->eglGetError());
        return false;
    }

    mEglInitialized = true;
    return true;
}

ScopedEglContext VirtualSceneCameraDevice::makeEglCurrent() {
    const EGLBoolean result = mEglDispatch->eglMakeCurrent(
            mEglDisplay, mEglSurface, mEglSurface, mEglContext);
    if (result == EGL_FALSE) {
        E("%s: eglMakeCurrent failed, error %d", __FUNCTION__,
          mEglDispatch->eglGetError());

        // Explicitly set mEglInitialized to false here to avoid an infinite
        // loop with stopCapturing.
        mEglInitialized = false;
        stopCapturing();

        // Return an empty ScopedEglContext because we failed to call
        // eglMakeCurrent.
        return ScopedEglContext(nullptr, EGL_NO_DISPLAY);
    }

    return ScopedEglContext(mEglDispatch, mEglDisplay);
}

}  // namespace virtualscene
}  // namespace android

/*******************************************************************************
 *                     CameraDevice API
 ******************************************************************************/

using android::virtualscene::VirtualSceneCameraDevice;
using android::virtualscene::VirtualSceneManager;

static VirtualSceneCameraDevice* toVirtualSceneCameraDevice(CameraDevice* ccd) {
    if (!ccd || !ccd->opaque) {
        return nullptr;
    }

    return reinterpret_cast<VirtualSceneCameraDevice*>(ccd->opaque);
}

void camera_virtualscene_parse_cmdline() {
    VirtualSceneManager::parseCmdline();
}

uint32_t camera_virtualscene_preferred_format() {
    return VIRTUALSCENE_PIXEL_FORMAT;
}

CameraDevice* camera_virtualscene_open(const char* name, int inp_channel) {
    VirtualSceneCameraDevice* cd = new VirtualSceneCameraDevice();
    return cd ? cd->getCameraDevice() : nullptr;
}

int camera_virtualscene_start_capturing(CameraDevice* ccd,
                                        uint32_t pixel_format,
                                        int frame_width,
                                        int frame_height) {
    VirtualSceneCameraDevice* cd = toVirtualSceneCameraDevice(ccd);
    if (!cd) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }

    return cd->startCapturing(pixel_format, frame_width, frame_height);
}

int camera_virtualscene_stop_capturing(CameraDevice* ccd) {
    VirtualSceneCameraDevice* cd = toVirtualSceneCameraDevice(ccd);
    if (!cd) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }

    cd->stopCapturing();

    return 0;
}

int camera_virtualscene_read_frame(CameraDevice* ccd,
                                   ClientFrame* result_frame,
                                   float r_scale,
                                   float g_scale,
                                   float b_scale,
                                   float exp_comp) {
    VirtualSceneCameraDevice* cd = toVirtualSceneCameraDevice(ccd);
    if (!cd) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return -1;
    }

    return cd->readFrame(result_frame, r_scale, g_scale, b_scale, exp_comp);
}

void camera_virtualscene_close(CameraDevice* ccd) {
    VirtualSceneCameraDevice* cd = toVirtualSceneCameraDevice(ccd);
    if (!cd) {
        E("%s: Invalid camera device descriptor", __FUNCTION__);
        return;
    }

    delete cd;
}
