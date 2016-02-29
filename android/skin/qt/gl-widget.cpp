// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/gl-widget.h"

#include "android/base/memory/LazyInstance.h"

#include "GLES2/gl2.h"

#include <QResizeEvent>
#include <QtGlobal>

// Note that this header must come after ALL Qt headers.
// Including any Qt headers after it will cause compilation
// to fail spectacularly. This is because EGL indirectly
// includes X11 headers which define stuff that conflicts
// with Qt's own macros. It is a known issue.
#include "OpenGLESDispatch/EGLDispatch.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

namespace {

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

// Must be declared outside of MyGLESv2Dispatch scope due to the use of
// sizeof(T) within the template definition.
android::base::LazyInstance<MyGLESv2Dispatch> sGLESv2Dispatch =
            LAZY_INSTANCE_INIT;


// static
const GLESv2Dispatch* MyGLESv2Dispatch::get() {
    MyGLESv2Dispatch* instance = sGLESv2Dispatch.ptr();
    if (instance->mValid) {
        return &instance->mDispatch;
    } else {
        return nullptr;
    }
}

// Helper class used to laziy initialize the global EGL dispatch table
// in a thread safe way. Note that the dispatch table is provided by
// libOpenGLESDispatch as the 's_egl' global variable.
struct MyEGLDispatch : public EGLDispatch {
    // Return pointer to EGLDispatch table, or nullptr if there was
    // an error when trying to initialize/load the library.
    static const EGLDispatch* get();

    MyEGLDispatch() {
        mValid = init_egl_dispatch();
    }

private:
    bool mValid;
};

android::base::LazyInstance<MyEGLDispatch> sEGLDispatch =
        LAZY_INSTANCE_INIT;

// static
const EGLDispatch* MyEGLDispatch::get() {
    MyEGLDispatch* instance = sEGLDispatch.ptr();
    if (instance->mValid) {
        return &s_egl;
    } else {
        return nullptr;
    }
}

}  // namespace

struct EGLState {
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
};


// Helper macro for checking error status and cleaning up.
#define CHECK_GL_ERROR(error_msg, cleanup_func) if (mGLES2->glGetError() != GL_NO_ERROR) { \
    qWarning(error_msg); \
    cleanup_func(); \
    return; \
}

// Vertex shader for anti-aliasing - doesn't do anything special.
static const char AAVertexShaderSource[] =
    "attribute vec3 position;\n"
    "attribute vec2 inCoord;\n"
    "varying vec2 outCoord;\n"

    "void main(void) {\n"
    "    gl_Position.x = position.x;\n"
    "    gl_Position.y = position.y;\n"
    "    gl_Position.z = position.z;\n"
    "    gl_Position.w = 1;\n"
    "    outCoord = inCoord;\n"
    "}\n";

// Fragment shader - does fast approximate anti-aliasing (FXAA)
// Based on:
// http://www.geeks3d.com/20110405/fxaa-fast-approximate-anti-aliasing-demo-glsl-opengl-test-radeon-geforce/
static const char AAFragmentShaderSource[] =
    "varying lowp vec2 outCoord;\n"
    "uniform sampler2D texture;\n"
    "uniform vec2 resolution;\n"
    "float luma(vec3 color) {\n"
    "    return dot(color, vec3(0.299, 0.587, 0.114));"
    "}\n"
    "vec4 antialias(sampler2D tex,\n"
    "               vec2 pixel_coord,\n"
    "               vec2 inverse_res,\n"
    "               vec2 nw, vec2 ne, \n"
    "               vec2 sw, vec2 se, \n"
    "               vec2 m) {\n"
    "    vec4 m_rgba = texture2D(tex, m);"
    "    vec3 nw_color = texture2D(tex, nw).xyz;\n"
    "    vec3 ne_color = texture2D(tex, ne).xyz;\n"
    "    vec3 sw_color = texture2D(tex, sw).xyz;\n"
    "    vec3 se_color = texture2D(tex, se).xyz;\n"
    "    vec3 m_color  = m_rgba.xyz;\n"
    "    float nw_luma = luma(nw_color);\n"
    "    float ne_luma = luma(ne_color);\n"
    "    float sw_luma = luma(sw_color);\n"
    "    float se_luma = luma(se_color);\n"
    "    float m_luma  = luma(m_color);\n"
    "    float min_luma = min(m_luma, min(min(nw_luma, ne_luma), min(sw_luma, se_luma)));\n"
    "    float max_luma = max(m_luma, max(max(nw_luma, ne_luma), max(sw_luma, se_luma)));;\n"
    "    \n"
    "    mediump vec2 dir;\n"
    "    dir.x = -((nw_luma + ne_luma) - (sw_luma + se_luma));\n"
    "    dir.y =  ((nw_luma + sw_luma) - (ne_luma + se_luma));\n"
    "    \n"
    "    float dir_reduce = max((nw_luma + ne_luma + sw_luma + se_luma) *\n"
    "                          (0.25 * (1.0 / 8.0)), (1.0/256.0));\n"
    "    \n"
    "    float rcp_dir_min = 1.0 / (min(abs(dir.x), abs(dir.y)) + dir_reduce);\n"
    "    dir = min(vec2(16.0, 16.0),\n"
    "              max(vec2(-16.0, -16.0),\n"
    "              dir * rcp_dir_min)) * inverse_res;\n"
    "    \n"
    "    vec2 tex_coord = pixel_coord * inverse_res;\n"
    "    vec3 rgb_a = 0.5 * (\n"
    "        texture2D(tex, tex_coord + dir * (1.0 / 3.0 - 0.5)).xyz +\n"
    "        texture2D(tex, tex_coord + dir * (2.0 / 3.0 - 0.5)).xyz);\n"
    "    vec3 rgb_b = rgb_a * 0.5 + 0.25 * (\n"
    "        texture2D(tex, tex_coord + dir * -0.5).xyz +\n"
    "        texture2D(tex, tex_coord+ dir * 0.5).xyz);\n"
    "\n"
    "    float luma_b = luma(rgb_b);\n"
    "    return vec4((luma_b < min_luma) || (luma_b > max_luma) ? rgb_a  : rgb_a, m_rgba.a);\n"
    "}\n"
    "void main(void) {\n"
    "    vec2 pixel_coord = outCoord * resolution;\n"
    "    vec2 inverse_res = 1.0 / resolution;\n"
    "    vec2 nw = (pixel_coord + vec2(-1.0, -1.0)) * inverse_res;\n"
    "    vec2 ne = (pixel_coord + vec2( 1.0, -1.0)) * inverse_res;\n"
    "    vec2 sw = (pixel_coord + vec2(-1.0,  1.0)) * inverse_res;\n"
    "    vec2 se = (pixel_coord + vec2( 1.0,  1.0)) * inverse_res;\n"
    "    vec2 m = pixel_coord * inverse_res;\n"
    "    gl_FragColor = antialias(texture, pixel_coord, inverse_res, nw, ne, sw, se, m);\n"
    "}\n";

GLWidget::GLWidget(QWidget* parent) :
        QWidget(parent),
        mEGL(MyEGLDispatch::get()),
        mGLES2(MyGLESv2Dispatch::get()),
        mEGLState(nullptr),
        mValid(false),
        mFramebuffer(0),
        mTargetTexture(0),
        mDepthTexture(0),
        mAAProgram(0),
        mAAVertexBuffer(0),
        mAAIndexBuffer(0) {
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_PaintOnScreen, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
}

bool GLWidget::ensureInit() {
    // If an error occured when loading the EGL/GLESv2 libraries, return false.
    if (!mEGL || !mGLES2) {
        return false;
    }

    // If already initialized, return mValid to indicate if an error occured.
    if (mEGLState) {
        return mValid;
    }

    mEGLState = new EGLState();
    mValid = false;

    mEGLState->display = mEGL->eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (mEGLState->display == EGL_NO_DISPLAY) {
        qWarning("Failed to get EGL display");
        return false;
    }

    EGLint egl_maj, egl_min;
    EGLConfig egl_config;

    // Try to initialize EGL display.
    // Initializing an already-initialized display is OK.
    if (mEGL->eglInitialize(mEGLState->display,
                            &egl_maj,
                            &egl_min) == EGL_FALSE) {
        qWarning("Failed to initialize EGL display");
        return false;
    }

    // Get an EGL config.
    const EGLint config_attribs[] =
        {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_SAMPLES, 0, // No multisampling
            EGL_NONE
        };
    EGLint num_config;
    EGLBoolean choose_result =
        mEGL->eglChooseConfig(
            mEGLState->display,
            config_attribs,
            &egl_config,
            1,
            &num_config);
    if (choose_result == EGL_FALSE || num_config < 1) {
        qWarning("Failed to choose EGL config");
        return false;
    }

    // Create a context.
    EGLint context_attribs[] =
        {
            EGL_CONTEXT_CLIENT_VERSION, 2,
            EGL_NONE
        };
    mEGLState->context = mEGL->eglCreateContext(
            mEGLState->display,
            egl_config,
            EGL_NO_CONTEXT,
            context_attribs);
    if (mEGLState->context == EGL_NO_CONTEXT) {
        qWarning("Failed to create EGL context %d", mEGL->eglGetError());
    }

    // Finally, create a window surface associated with this widget.
    mEGLState->surface = mEGL->eglCreateWindowSurface(
            mEGLState->display,
            egl_config,
            (EGLNativeWindowType)(winId()),
            nullptr);
    if (mEGLState->surface == EGL_NO_SURFACE) {
        qWarning("Failed to create an EGL surface %d", mEGL->eglGetError());
        return false;
    }

    mValid = true;

    makeContextCurrent();
    initFramebuffer();
    initAAProgram();
    bindFramebuffer();
    initGL();
    unbindFramebuffer();

    return true;
}

void GLWidget::makeContextCurrent() {
    mEGL->eglMakeCurrent(
            mEGLState->display,
            mEGLState->surface,
            mEGLState->surface,
            mEGLState->context);
}

void GLWidget::renderFrame() {
    if (!ensureInit()) {
        return;
    }
    makeContextCurrent();

    // Render 3D scene to texture.
    bindFramebuffer();
    repaintGL();
    unbindFramebuffer();

    // Draw texture on-screen, applying the anti-aliasing shader.
    mGLES2->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mGLES2->glUseProgram(mAAProgram);
    mGLES2->glBindBuffer(GL_ARRAY_BUFFER, mAAVertexBuffer);
    mGLES2->glEnableVertexAttribArray(mAAPositionAttribLocation);
    mGLES2->glVertexAttribPointer(mAAPositionAttribLocation,
                                  3,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(float) * 5,
                                  0);
    mGLES2->glEnableVertexAttribArray(mAATexCoordAttribLocation);
    mGLES2->glVertexAttribPointer(mAATexCoordAttribLocation,
                                  2,
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(float) * 5,
                                  reinterpret_cast<GLvoid*>(
                                        static_cast<uintptr_t>(
                                                sizeof(float) * 3)));
    mGLES2->glActiveTexture(GL_TEXTURE0);
    mGLES2->glBindTexture(GL_TEXTURE_2D, mTargetTexture);
    mGLES2->glUniform1i(mAAInputUniformLocation, 0);
    mGLES2->glUniform2f(mAAResolutionUniformLocation,
                        width() * devicePixelRatio(),
                        height() * devicePixelRatio());
    mGLES2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mAAIndexBuffer);
    mGLES2->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, 0);

    mEGL->eglSwapBuffers(mEGLState->display, mEGLState->surface);
}

void GLWidget::paintEvent(QPaintEvent*) {
    renderFrame();
}

void GLWidget::showEvent(QShowEvent*) {
    renderFrame();
}

void GLWidget::resizeEvent(QResizeEvent* e) {
    if (mEGLState) {
        // We should only call resizeGL and repaint if all the setup
        // has been done.
        // We should NOT attempt to initialize EGL state during a resize
        // event due to some subtleties on OS X. If we attempt to initialize
        // EGL state at the time the resize event is generated, the default
        // framebuffer will not be created.
        makeContextCurrent();
        // Re-create the framebuffer with new size.
        destroyFramebuffer();
        initFramebuffer();
        bindFramebuffer();
        resizeGL(e->size().width(), e->size().height());
        repaintGL();
        mEGL->eglSwapBuffers(mEGLState->display, mEGLState->surface);
    }
}

void GLWidget::initFramebuffer() {
    // Create and bind framebuffer object.
    mGLES2->glGenFramebuffers(1, &mFramebuffer);
    CHECK_GL_ERROR("Failed to create framebuffer object", destroyFramebuffer);
    mGLES2->glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    CHECK_GL_ERROR("Failed to bind framebuffer object", destroyFramebuffer);

    // Create a texture to render to.
    mGLES2->glGenTextures(1, &mTargetTexture);
    CHECK_GL_ERROR("Failed to create target texture for FBO", destroyFramebuffer);
    mGLES2->glBindTexture(GL_TEXTURE_2D, mTargetTexture);
    mGLES2->glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGB,
                         width() * devicePixelRatio(),
                         height() * devicePixelRatio(),
                         0,
                         GL_RGB,
                         GL_UNSIGNED_BYTE,
                         0);
    CHECK_GL_ERROR("Failed to populate target texture", destroyFramebuffer);
    mGLES2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    mGLES2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    mGLES2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    mGLES2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    mGLES2->glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_2D,
                                   mTargetTexture,
                                   0);
    CHECK_GL_ERROR("Failed to set FBO color attachment", destroyFramebuffer);


    // Create a texture for the depth buffer.
    mGLES2->glGenTextures(1, &mDepthTexture);
    CHECK_GL_ERROR("Failed to create depth texture", destroyFramebuffer);
    mGLES2->glBindTexture(GL_TEXTURE_2D, mDepthTexture);
    mGLES2->glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_DEPTH_COMPONENT,
                         width() * devicePixelRatio(),
                         height() * devicePixelRatio(),
                         0,
                         GL_DEPTH_COMPONENT,
                         GL_UNSIGNED_INT,
                         0);
    CHECK_GL_ERROR("Failed to populate depth texture", destroyFramebuffer);
    mGLES2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    mGLES2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    mGLES2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    mGLES2->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    mGLES2->glFramebufferTexture2D(GL_FRAMEBUFFER,
                                   GL_DEPTH_ATTACHMENT,
                                   GL_TEXTURE_2D,
                                   mDepthTexture,
                                   0);
    CHECK_GL_ERROR("Failed to set FBO depth attachment", destroyFramebuffer);

    // Verify that FBO is valid.
    if (mGLES2->glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
            GL_FRAMEBUFFER_COMPLETE) {
        qWarning("FBO not complete");
        destroyFramebuffer();
    }

    // Restore the default frame-buffer.
    mGLES2->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Helper for GLWidget::initAAProgram
static GLuint createShader(const GLESv2Dispatch* gles2, GLint shaderType, const char* shaderText) {
    GLuint shader = gles2->glCreateShader(shaderType);
    if (!shader) {
        return 0;
    }
    const GLchar* text = static_cast<const GLchar*>(shaderText);
    const GLint textLen = ::strlen(shaderText);
    gles2->glShaderSource(shader, 1, &text, &textLen);
    gles2->glCompileShader(shader);
    GLint success;
    gles2->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char i[256];
        gles2->glGetShaderInfoLog(shader, 256, NULL, i);
        qWarning("DIED %s", i);
    }
    return success == GL_TRUE ? shader : 0;
}

void GLWidget::initAAProgram() {
    // Build the anti-aliasing shader.
    GLuint vertex_shader =
        createShader(mGLES2, GL_VERTEX_SHADER, AAVertexShaderSource);
    CHECK_GL_ERROR("Failed to create vertex shader for anti-aliasing",
                   destroyAAProgram);
    GLuint fragment_shader =
        createShader(mGLES2, GL_FRAGMENT_SHADER, AAFragmentShaderSource);
    CHECK_GL_ERROR("Failed to create fragment shader for anti-aliasing",
                   destroyAAProgram);
    mAAProgram = mGLES2->glCreateProgram();
    CHECK_GL_ERROR("Failed to create program object", destroyAAProgram);
    mGLES2->glAttachShader(mAAProgram, vertex_shader);
    mGLES2->glAttachShader(mAAProgram, fragment_shader);
    mGLES2->glLinkProgram(mAAProgram);

    // Shader objects no longer needed.
    mGLES2->glDeleteShader(vertex_shader);
    mGLES2->glDeleteShader(fragment_shader);

    // Check for errors.
    GLint success;
    mGLES2->glGetProgramiv(mAAProgram, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar infolog[256];
        mGLES2->glGetProgramInfoLog(
                mAAProgram, sizeof(infolog), 0, infolog);
        qWarning("Could not create/link program: %s\n", infolog);
        destroyAAProgram();
        return;
    }

    // Get all the attributes and uniforms.
    mAAPositionAttribLocation = mGLES2->glGetAttribLocation(mAAProgram, "position");
    mAATexCoordAttribLocation = mGLES2->glGetAttribLocation(mAAProgram, "inCoord");
    mAAInputUniformLocation = mGLES2->glGetUniformLocation(mAAProgram, "texture");
    mAAResolutionUniformLocation = mGLES2->glGetUniformLocation(mAAProgram, "resolution");
    CHECK_GL_ERROR("Failed to get attributes & uniforms for anti-aliasing shader",
                   destroyAAProgram);

    // Create vertex and index buffers.
    mGLES2->glGenBuffers(1, &mAAVertexBuffer);
    CHECK_GL_ERROR("Failed to create vertex buffer for anti-aliasing shader",
                   destroyAAProgram);
    mGLES2->glBindBuffer(GL_ARRAY_BUFFER, mAAVertexBuffer);
    static const float vertex_data[] = {
        +1, -1, +0,  +1, +0,
        +1, +1, +0,  +1, +1,
        -1, +1, +0,  +0, +1,
        -1, -1, +0,  +0, +0,
    };
    mGLES2->glBufferData(
            GL_ARRAY_BUFFER,
            sizeof(vertex_data),
            vertex_data,
            GL_STATIC_DRAW);
    CHECK_GL_ERROR("Failed to populate vertex buffer for anti-aliasing shader",
                   destroyAAProgram);

    mGLES2->glGenBuffers(1, &mAAIndexBuffer);
    CHECK_GL_ERROR("Failed to create index buffer for anti-aliasing shader",
                   destroyAAProgram);
    mGLES2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mAAIndexBuffer);
    static const GLubyte index_data[] = { 0, 1, 2, 2, 3, 0 };
    mGLES2->glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                         sizeof(index_data),
                         index_data,
                         GL_STATIC_DRAW);
    CHECK_GL_ERROR("Failed to populate index buffer for anti-aliasing shader",
                   destroyAAProgram);
}

void GLWidget::bindFramebuffer() {
    mGLES2->glBindFramebuffer(GL_FRAMEBUFFER, mFramebuffer);
    mGLES2->glViewport(
        0,
        0,
        width() * devicePixelRatio(),
        height() * devicePixelRatio());
}

void GLWidget::unbindFramebuffer() {
    mGLES2->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLWidget::destroyFramebuffer() {
    if (mFramebuffer) {
        mGLES2->glDeleteFramebuffers(1, &mFramebuffer);
        mFramebuffer = 0;
    }

    if (mTargetTexture) {
        mGLES2->glDeleteTextures(1, &mTargetTexture);
        mTargetTexture = 0;
    }

    if (mDepthTexture) {
        mGLES2->glDeleteTextures(1, &mDepthTexture);
        mDepthTexture = 0;
    }
}

void GLWidget::destroyAAProgram() {
    if (mAAProgram) {
        mGLES2->glUseProgram(0);
        mGLES2->glDeleteProgram(mAAProgram);
        mAAProgram = 0;
    }

    if (mAAVertexBuffer) {
        mGLES2->glBindBuffer(GL_ARRAY_BUFFER, 0);
        mGLES2->glDeleteBuffers(1, &mAAVertexBuffer);
        mAAVertexBuffer = 0;
    }

    if (mAAIndexBuffer) {
        mGLES2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        mGLES2->glDeleteBuffers(1, &mAAIndexBuffer);
        mAAIndexBuffer = 0;
    }
}

GLWidget::~GLWidget() {
    if (mEGL && mEGLState) {
        mEGL->eglDestroyContext(mEGLState->display, mEGLState->context);
        mEGL->eglDestroySurface(mEGLState->display, mEGLState->surface);
        delete mEGLState;
    }
    unbindFramebuffer();
    destroyFramebuffer();
    destroyAAProgram();
}
