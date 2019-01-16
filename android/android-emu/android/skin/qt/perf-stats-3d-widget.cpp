// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/perf-stats-3d-widget.h"
#include "OpenGLESDispatch/GLESv2Dispatch.h"

#include "android/base/CpuUsage.h"
#include "android/base/system/System.h"
#include "android/base/Log.h"
#include "android/skin/qt/gl-common.h"

#include <QTimer>

#include <string.h>

using namespace android::base;

PerfStats3DWidget::PerfStats3DWidget(QWidget* parent)
    : GLWidget(parent) {
    setFocusPolicy(Qt::ClickFocus);

    connect(&mCollectionTimer, SIGNAL(timeout()),
            this, SLOT(updateAndRefreshValues()));

    mCollectionTimer.setInterval(kCollectionIntervalMs);
}

PerfStats3DWidget::~PerfStats3DWidget() {
    // Free up allocated resources.
    if (!mGLES2) {
        return;
    }
    if (readyForRendering()) {
        if(makeContextCurrent()) {
            mGLES2->glDeleteProgram(mProgram);
            mGLES2->glDeleteBuffers(1, &mYAxisBuffer);
            mGLES2->glDeleteBuffers(1, &mXAxisBuffer);
        }
    }
}

void PerfStats3DWidget::onCollectionEnabled() {
    CpuUsage::get()->setMeasurementInterval(kCollectionIntervalMs * 1000);
    mCollectionTimer.start();
}

void PerfStats3DWidget::onCollectionDisabled() {
    mCollectionTimer.stop();
    CpuUsage::get()->setMeasurementInterval(CpuUsage::kDefaultMeasurementIntervalUs);
}

void PerfStats3DWidget::updateAndRefreshValues() {
    if (!mDataAvailable) return;

    if (!mGLES2) {
        return;
    }

    auto cpuUsage = CpuUsage::get();

    cpuUsage->setMeasurementInterval(16000);

    float totalCpu = 0.0f;

    cpuUsage->forEachUsage(CpuUsage::UsageArea::MainLoop, [&totalCpu](const CpuTime& t) {
        totalCpu += t.usage();
    });

    cpuUsage->forEachUsage(CpuUsage::UsageArea::Vcpu, [&totalCpu](const CpuTime& t) {
        totalCpu += t.usage();
    });

    memmove(mYValues.data(), mYValues.data() + 1, sizeof(float) * (mYValues.size() - 1));
    mYValues[mYValues.size() - 1] = totalCpu / 4.0f;

    refreshData();
    renderFrame();
}

bool PerfStats3DWidget::initGL() {
    if (!mGLES2) {
        return false;
    }

    // Perform initial set-up.
    auto gl = mGLES2;

    // Enable depth testing and blending.
    gl->glEnable(GL_DEPTH_TEST);
    gl->glEnable(GL_BLEND);

    // Set the blend function.
    gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Clear screen to dark theme bg
    float clearR = 0.15294117647058825;
    float clearG = 0.19607843137254902;
    float clearB = 0.2196078431372549;
    gl->glClearColor(clearR, clearG, clearB, 1.0);

    if (!initProgram()) {
        LOG(ERROR) << "Could not initialize shader program";
    }

    if (!initGraphLines()) {
        LOG(ERROR) << "Could not initialize graph line models";
    }

    resizeGL(width(), height());
    return true;
}

bool PerfStats3DWidget::initProgram() {
    auto gl = mGLES2;

    // Compile & link shaders.
    constexpr char vShaderSrc[] = R"(precision highp float;
attribute float xvalue;
attribute float yvalue;

void main() {
    gl_Position = vec4(xvalue, 2.0 * yvalue - 1.0, 0.0, 1.0);
})";

    constexpr char fShaderSrc[] = R"(precision highp float;
uniform vec4 graphColor;
void main() {
    gl_FragColor = vec4(0,1,0,1);
})";

    GLuint vertex_shader =
        createShader(gl, GL_VERTEX_SHADER, vShaderSrc);
    GLuint fragment_shader =
        createShader(gl, GL_FRAGMENT_SHADER, fShaderSrc);

    if (vertex_shader == 0 || fragment_shader == 0) {
        LOG(ERROR) << "Could not initialize graph shaders";
        return false;
    }

    mProgram = gl->glCreateProgram();
    gl->glAttachShader(mProgram, vertex_shader);
    gl->glAttachShader(mProgram, fragment_shader);

    gl->glBindAttribLocation(mProgram, 0, "xvalue");
    gl->glBindAttribLocation(mProgram, 1, "yvalue");

    gl->glLinkProgram(mProgram);

    GLint compile_status = 0;

    gl->glGetProgramiv(mProgram, GL_LINK_STATUS, &compile_status);

    if (compile_status == GL_FALSE) {
        qWarning("Failed to link program");
        return false;
    }

    CHECK_GL_ERROR_RETURN("Failed to initialize shaders", false);

    // We no longer need shader objects after successful program linkage.
    gl->glDetachShader(mProgram, vertex_shader);
    gl->glDetachShader(mProgram, fragment_shader);
    gl->glDeleteShader(vertex_shader);
    gl->glDeleteShader(fragment_shader);

    return true;
}

bool PerfStats3DWidget::initGraphLines() {
    auto gl = mGLES2;

    gl->glGenBuffers(1, &mXAxisBuffer);
    gl->glGenBuffers(1, &mYAxisBuffer);

    gl->glBindBuffer(GL_ARRAY_BUFFER, mXAxisBuffer);

    std::vector<float> xValues(mCollectionHistory);
    for (uint32_t i = 0; i < mCollectionHistory; ++i) {
        xValues[i] = 2.0f * float(i) / mCollectionHistory - 1.0f;
    }

    gl->glBufferData(GL_ARRAY_BUFFER, xValues.size() * sizeof(float), xValues.data(), GL_STREAM_DRAW);

    gl->glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);

    gl->glBindBuffer(GL_ARRAY_BUFFER, mYAxisBuffer);

    mYValues.resize(mCollectionHistory);
    memset(mYValues.data(), 0x0, sizeof(float) * mYValues.size());

    gl->glBufferData(GL_ARRAY_BUFFER, mYValues.size() * sizeof(float), mYValues.data(), GL_STREAM_DRAW);

    refreshData();

    gl->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);

    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);

    CHECK_GL_ERROR_RETURN("Failed to initialize graph lines", false);

    mDataAvailable = true;

    return true;
}

void PerfStats3DWidget::refreshData() {
    if (!mDataAvailable) return;

    if (!mGLES2) {
        return;
    }

    auto gl = mGLES2;

    gl->glBindBuffer(GL_ARRAY_BUFFER, mYAxisBuffer);
    gl->glBufferSubData(GL_ARRAY_BUFFER, 0, mYValues.size() * sizeof(float), mYValues.data());
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void PerfStats3DWidget::resizeGL(int w, int h) {
    if (!mGLES2) {
        return;
    }

    // Adjust for new viewport
    mGLES2->glViewport(0, 0, w, h);
}

void PerfStats3DWidget::repaintGL() {
    if (!mGLES2) {
        return;
    }

    auto gl = mGLES2;

    gl->glUseProgram(mProgram);

    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl->glEnableVertexAttribArray(0);
    gl->glEnableVertexAttribArray(1);
    gl->glDrawArrays(GL_LINE_STRIP, 0, mCollectionHistory);
    gl->glDisableVertexAttribArray(0);
    gl->glDisableVertexAttribArray(1);
}

void PerfStats3DWidget::mouseMoveEvent(QMouseEvent* event) {
}

void PerfStats3DWidget::wheelEvent(QWheelEvent* event) {
}

void PerfStats3DWidget::mousePressEvent(QMouseEvent* event) {
}

void PerfStats3DWidget::mouseReleaseEvent(QMouseEvent* event) {
}

