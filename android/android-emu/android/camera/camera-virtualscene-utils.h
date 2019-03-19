/*
 * Copyright (C) 2019 The Android Open Source Project
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

#pragma once

#include <memory>

#include "android/base/Log.h"
#include "android/base/memory/LazyInstance.h"
#include "android/camera/camera-format-converters.h"
#include "android/virtualscene/VirtualSceneManager.h"
#include "emugl/common/OpenGLDispatchLoader.h"


namespace android {
namespace virtualscene {

// Defines an RAII object to automatically unset the EGL context when the scope
// exits. Returned by RenderedCameraDevice::makeEglCurrent.
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
                LOG(WARNING) << "Could not unset eglMakeCurrent error "
                          << mEglDispatch->eglGetError();
            }
        }
    }

    bool isValid() const { return mEglDispatch; }

private:
    const EGLDispatch* mEglDispatch;
    EGLDisplay mEglDisplay;
};

// Abstract class for rendering scenes or frames to the emulator camera.
class CameraRenderer {
public:
    CameraRenderer() = default;
    virtual ~CameraRenderer() = default;
    virtual bool initialize(const GLESv2Dispatch* gles2,
                            int width,
                            int height) = 0;
    virtual void uninitialize() = 0;
    virtual int64_t render() = 0;
};


/*******************************************************************************
 *                     RenderedCameraDevice routines
 ******************************************************************************/

/*
 * Describes a connection to an actual camera device.
 */
class RenderedCameraDevice {
    DISALLOW_COPY_AND_ASSIGN(RenderedCameraDevice);

public:
    RenderedCameraDevice(std::unique_ptr<CameraRenderer> renderer);
    ~RenderedCameraDevice();

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
    // thread-local EGL state to make the RenderedCameraDevice's EGL context
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
    std::unique_ptr<CameraRenderer> renderer;
};

}  // namespace virtualscene
}  // namespace android
