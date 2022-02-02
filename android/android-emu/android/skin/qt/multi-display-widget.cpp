// Copyright 2021 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.


#include "android/skin/qt/multi-display-widget.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"  // for GLESv2Dispatch
#include "android/base/Log.h"                 // for LOG
#include "android/opengl/virtio_gpu_ops.h"
#include "android/opengles.h"
#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/gl-common.h"

MultiDisplayWidget::MultiDisplayWidget(uint32_t frameWidth,
                                       uint32_t frameHeight,
                                       uint32_t displayId,
                                       QWidget* parent)
    : GLWidget(parent),
      mFrameWidth(frameWidth),
      mFrameHeight(frameHeight),
      mDisplayId(displayId),
      mColorBufferId(0) {
    Qt::WindowFlags flags = windowFlags();
    flags |= Qt::CustomizeWindowHint;
    flags &= ~Qt::WindowCloseButtonHint;
    setWindowFlags(flags);
}

MultiDisplayWidget::~MultiDisplayWidget() {
    clearGL();
}

const char kVertexShaderSource[] =
        "attribute vec4 position;\n"
        "attribute vec2 inCoord;\n"
        "varying vec2 outCoord;\n"
        "uniform vec2 translation;\n"
        "uniform vec2 scale;\n"
        "uniform vec2 coordTranslation;\n"
        "uniform vec2 coordScale;\n"

        "void main(void) {\n"
        "  gl_Position.xy = position.xy * scale.xy - translation.xy;\n"
        "  gl_Position.zw = position.zw;\n"
        "  outCoord = inCoord * coordScale + coordTranslation;\n"
        "}\n";

// Similarly, just interpolate texture coordinates.
const char kFragmentShaderSource[] =
        "precision mediump float;\n"
        "varying lowp vec2 outCoord;\n"
        "uniform sampler2D tex;\n"
        "uniform float alpha;\n"

        "void main(void) {\n"
        "  gl_FragColor = alpha * texture2D(tex, outCoord);\n"
        "}\n";

// Hard-coded arrays of vertex information.
struct Vertex {
    float pos[3];
    float coord[2];
};

const Vertex kVertices[] = {
        // 0 degree
        {{+1, -1, +0}, {+1, +0}},
        {{+1, +1, +0}, {+1, +1}},
        {{-1, +1, +0}, {+0, +1}},
        {{-1, -1, +0}, {+0, +0}},
        // 90 degree clock-wise
        {{+1, -1, +0}, {+1, +1}},
        {{+1, +1, +0}, {+0, +1}},
        {{-1, +1, +0}, {+0, +0}},
        {{-1, -1, +0}, {+1, +0}},
        // 180 degree clock-wise
        {{+1, -1, +0}, {+0, +1}},
        {{+1, +1, +0}, {+0, +0}},
        {{-1, +1, +0}, {+1, +0}},
        {{-1, -1, +0}, {+1, +1}},
        // 270 degree clock-wise
        {{+1, -1, +0}, {+0, +0}},
        {{+1, +1, +0}, {+1, +0}},
        {{-1, +1, +0}, {+1, +1}},
        {{-1, -1, +0}, {+0, +1}},
        // flip horizontally
        {{+1, -1, +0}, {+0, +0}},
        {{+1, +1, +0}, {+0, +1}},
        {{-1, +1, +0}, {+1, +1}},
        {{-1, -1, +0}, {+1, +0}},
        // flip vertically
        {{+1, -1, +0}, {+1, +1}},
        {{+1, +1, +0}, {+1, +0}},
        {{-1, +1, +0}, {+0, +0}},
        {{-1, -1, +0}, {+0, +1}},
        // flip source image horizontally, the rotate 90 degrees clock-wise
        {{+1, -1, +0}, {+0, +1}},
        {{+1, +1, +0}, {+1, +1}},
        {{-1, +1, +0}, {+1, +0}},
        {{-1, -1, +0}, {+0, +0}},
        // flip source image vertically, the rotate 90 degrees clock-wise
        {{+1, -1, +0}, {+1, +0}},
        {{+1, +1, +0}, {+0, +0}},
        {{-1, +1, +0}, {+0, +1}},
        {{-1, -1, +0}, {+1, +1}},
};

// Vertex indices for predefined rotation angles.
const GLubyte kIndices[] = {
        0,  1,  2,  2,  3,  0,   // 0
        4,  5,  6,  6,  7,  4,   // 90
        8,  9,  10, 10, 11, 8,   // 180
        12, 13, 14, 14, 15, 12,  // 270
        16, 17, 18, 18, 19, 16,  // flip h
        20, 21, 22, 22, 23, 20,  // flip v
        24, 25, 26, 26, 27, 24,  // flip h, 90
        28, 29, 30, 30, 31, 28   // flip v, 90
};

const GLint kIndicesLen = sizeof(kIndices) / sizeof(kIndices[0]);
const GLint kIndicesPerDraw = 6;

GLuint MultiDisplayWidget::createShader(GLint shaderType,
                                        const char* shaderText) {
    // Create new shader handle and attach source.
    GLuint shader = mGLES2->glCreateShader(shaderType);
    if (!shader) {
        return 0;
    }
    const GLchar* text = static_cast<const GLchar*>(shaderText);
    const GLint textLen = ::strlen(shaderText);
    mGLES2->glShaderSource(shader, 1, &text, &textLen);

    // Compiler the shader.
    GLint success;
    mGLES2->glCompileShader(shader);
    mGLES2->glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLint infoLogLength;
        mGLES2->glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        std::string infoLog(infoLogLength + 1, '\0');
        LOG(ERROR) << __func__ << ": TextureDraw shader compile failed";
        mGLES2->glGetShaderInfoLog(shader, infoLogLength, 0, &infoLog[0]);
        LOG(VERBOSE) << "Info log: " << infoLog;
        LOG(VERBOSE) << "Source: " << shaderText;
        mGLES2->glDeleteShader(shader);
    }

    return shader;
}

bool MultiDisplayWidget::initGL() {
    mVertexShader = createShader(GL_VERTEX_SHADER, kVertexShaderSource);
    mFragmentShader = createShader(GL_FRAGMENT_SHADER, kFragmentShaderSource);
    mProgram = mGLES2->glCreateProgram();
    mGLES2->glAttachShader(mProgram, mVertexShader);
    mGLES2->glAttachShader(mProgram, mFragmentShader);

    GLint success;
    mGLES2->glLinkProgram(mProgram);
    mGLES2->glGetProgramiv(mProgram, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar messages[256];
        mGLES2->glGetProgramInfoLog(mProgram, sizeof(messages), 0,
                                    &messages[0]);
        LOG(ERROR) << "Could not create/link program: " << messages;
        mGLES2->glDeleteProgram(mProgram);
        mProgram = 0;
        return false;
    }

    mGLES2->glUseProgram(mProgram);

    // Retrieve attribute/uniform locations.
    mPositionSlot = mGLES2->glGetAttribLocation(mProgram, "position");
    mGLES2->glEnableVertexAttribArray(mPositionSlot);

    mInCoordSlot = mGLES2->glGetAttribLocation(mProgram, "inCoord");
    mGLES2->glEnableVertexAttribArray(mInCoordSlot);

    mAlpha = mGLES2->glGetUniformLocation(mProgram, "alpha");
    mCoordTranslation =
            mGLES2->glGetUniformLocation(mProgram, "coordTranslation");
    mCoordScale = mGLES2->glGetUniformLocation(mProgram, "coordScale");
    mScaleSlot = mGLES2->glGetUniformLocation(mProgram, "scale");
    mTranslationSlot = mGLES2->glGetUniformLocation(mProgram, "translation");
    mTextureSlot = mGLES2->glGetUniformLocation(mProgram, "tex");
    // set default uniform values
    mGLES2->glUniform1f(mAlpha, 1.0);
    mGLES2->glUniform2f(mTranslationSlot, 0.0, 0.0);
    mGLES2->glUniform2f(mScaleSlot, 1.0, 1.0);
    mGLES2->glUniform2f(mCoordTranslation, 0.0, 0.0);
    mGLES2->glUniform2f(mCoordScale, 1.0, 1.0);

    // Create vertex and index buffers.
    mGLES2->glGenBuffers(1, &mVertexBuffer);
    mGLES2->glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    mGLES2->glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices,
                         GL_STATIC_DRAW);

    mGLES2->glGenBuffers(1, &mIndexBuffer);
    mGLES2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
    mGLES2->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(kIndices), kIndices,
                         GL_STATIC_DRAW);

    mGLES2->glGenTextures(1, &mFrameTexture);

    // Reset state.
    mGLES2->glUseProgram(0);
    mGLES2->glDisableVertexAttribArray(mPositionSlot);
    mGLES2->glDisableVertexAttribArray(mInCoordSlot);
    mGLES2->glBindBuffer(GL_ARRAY_BUFFER, 0);
    mGLES2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return true;
}

void MultiDisplayWidget::clearGL() {
    mGLES2->glUseProgram(0);
    mGLES2->glDeleteProgram(mProgram);
    mProgram = 0;

    mGLES2->glBindBuffer(GL_ARRAY_BUFFER, 0);
    mGLES2->glDeleteBuffers(1, &mVertexBuffer);
    mVertexBuffer = 0;

    mGLES2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    mGLES2->glDeleteBuffers(1, &mIndexBuffer);
    mIndexBuffer = 0;

    mGLES2->glBindTexture(GL_TEXTURE_2D, 0);
    mGLES2->glDeleteTextures(1, &mFrameTexture);
    mFrameTexture = 0;
}

void MultiDisplayWidget::repaintGL() {
    mGLES2->glUseProgram(mProgram);
    mGLES2->glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
    mGLES2->glEnableVertexAttribArray(mPositionSlot);
    mGLES2->glVertexAttribPointer(mPositionSlot, 3, GL_FLOAT, GL_FALSE,
                                  sizeof(Vertex), 0);
    mGLES2->glEnableVertexAttribArray(mInCoordSlot);
    mGLES2->glVertexAttribPointer(
            mInCoordSlot, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
            reinterpret_cast<GLvoid*>(
                    static_cast<uintptr_t>(sizeof(float) * 3)));

    mGLES2->glActiveTexture(GL_TEXTURE0);
    mGLES2->glBindTexture(GL_TEXTURE_2D, mFrameTexture);
    struct AndroidVirtioGpuOps* ops = android_getVirtioGpuOps();
    ops->bind_color_buffer_to_texture(mColorBufferId);
    mGLES2->glUniform1i(mTextureSlot, 0);
    mGLES2->glUniform2f(mTranslationSlot, 0, 0);

    // Do the rendering.
    mGLES2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
    intptr_t indexShift = 5 * kIndicesPerDraw;
    mGLES2->glClear(GL_COLOR_BUFFER_BIT);
    mGLES2->glDrawElements(GL_TRIANGLES, kIndicesPerDraw, GL_UNSIGNED_BYTE,
                           (const GLvoid*)indexShift);

    // Clean up
    mGLES2->glUseProgram(0);
    mGLES2->glDisableVertexAttribArray(mPositionSlot);
    mGLES2->glDisableVertexAttribArray(mInCoordSlot);
    mGLES2->glBindBuffer(GL_ARRAY_BUFFER, 0);
    mGLES2->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void MultiDisplayWidget::paintWindow(uint32_t colorBufferId) {
    mColorBufferId = colorBufferId;
    renderFrame();
}

void MultiDisplayWidget::mousePressEvent(QMouseEvent* event) {
    // Pen long press generates synthesized mouse events,
    // which need to be filtered out
    if (event->source() == Qt::MouseEventNotSynthesized) {
        SkinEventType eventType = mMouseEvHandler.translateMouseEventType(
                    kEventMouseButtonDown, event->button(), event->buttons());
        mMouseEvHandler.handleMouseEvent(eventType, mMouseEvHandler.getSkinMouseButton(event),
                         event->pos(), event->globalPos(), false, mDisplayId);
    }
}

void MultiDisplayWidget::mouseMoveEvent(QMouseEvent* event) {
    // Pen long press generates synthesized mouse events,
    // which need to be filtered out
    if (event->source() == Qt::MouseEventNotSynthesized) {
        SkinEventType eventType = mMouseEvHandler.translateMouseEventType(
                kEventMouseMotion, event->button(), event->buttons());

        mMouseEvHandler.handleMouseEvent(eventType, mMouseEvHandler.getSkinMouseButton(event),
                         event->pos(), event->globalPos(), false, mDisplayId);
    }
}

void MultiDisplayWidget::mouseReleaseEvent(QMouseEvent* event) {
    // Pen long press generates synthesized mouse events,
    // which need to be filtered out
    if (event->source() == Qt::MouseEventNotSynthesized) {
        SkinEventType eventType = mMouseEvHandler.translateMouseEventType(
                    kEventMouseButtonUp, event->button(), event->buttons());
        mMouseEvHandler.handleMouseEvent(eventType, mMouseEvHandler.getSkinMouseButton(event),
                         event->pos(), event->globalPos(), false, mDisplayId);
    }
}

bool MultiDisplayWidget::event(QEvent* ev) {
    if (ev->type() == QEvent::TouchBegin || ev->type() == QEvent::TouchUpdate ||
        ev->type() == QEvent::TouchEnd) {
        QTouchEvent* touchEvent = static_cast<QTouchEvent*>(ev);
        auto win = EmulatorQtWindow::getInstance();
        win->handleTouchPoints(*touchEvent, mDisplayId);
        return true;
    }
    return QWidget::event(ev);
}

void MultiDisplayWidget::keyPressEvent(QKeyEvent* event) {
    auto win = EmulatorQtWindow::getInstance();
    win->handleKeyEvent(kEventKeyDown, event);
}

void MultiDisplayWidget::keyReleaseEvent(QKeyEvent* event) {
    auto win = EmulatorQtWindow::getInstance();
    win->handleKeyEvent(kEventKeyUp, event);
}
