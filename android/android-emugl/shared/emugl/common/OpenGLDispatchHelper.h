#pragma once

#include "android/base/system/System.h"

#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

// Helper class to hold a global GLESv2Dispatch that is initialized lazily
// in a thread-safe way. The instance is leaked on program exit.
struct MyGLESv2Dispatch : public GLESv2Dispatch {
    // Return pointer to global GLESv2Dispatch instance, or nullptr if there
    // was an error when trying to initialize/load the library.
    static const GLESv2Dispatch* get();

    MyGLESv2Dispatch() {
        mValid = gles2_dispatch_init(&mDispatch);
    }

private:
    GLESv2Dispatch mDispatch;
    bool mValid;
};

// Helper class used to lazily initialize the global EGL dispatch table
// in a thread safe way. Note that the dispatch table is provided by
// libOpenGLESDispatch as the 's_egl' global variable.
struct MyEGLDispatch : public EGLDispatch {
    // Return pointer to EGLDispatch table, or nullptr if there was
    // an error when trying to initialize/load the library.
    static const EGLDispatch* get();

    MyEGLDispatch() { mValid = init_egl_dispatch(); }

private:
    bool mValid;
};
