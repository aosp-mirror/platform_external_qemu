#pragma once

#include "emugl/common/OpenGLDispatchHelper.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl31.h>

namespace gltest {

EGLDisplay getDisplay();
EGLConfig createConfig(EGLDisplay dpy, EGLint r, EGLint g, EGLint b, EGLint a, EGLint d, EGLint s, EGLint ms);
EGLSurface pbufferSurface(EGLDisplay dpy, ::EGLConfig config, EGLint w, EGLint h);
EGLContext createContext(EGLDisplay dpy, EGLConfig config, EGLint maj, EGLint min);
void destroyContext(EGLDisplay dpy, EGLContext cxt);
void destroySurface(EGLDisplay dpy, EGLSurface surface);
void destroyDisplay(EGLDisplay dpy);

}
