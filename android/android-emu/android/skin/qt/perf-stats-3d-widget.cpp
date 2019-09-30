// Copyright 2019 The Android Open Source Project
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

#include "android/skin/qt/perf-stats-3d-widget.h"

#include <ctype.h>                              // for toupper
#include <qloggingcategory.h>                   // for qCWarning
#include <qnamespace.h>                         // for ClickFocus
#include <stdint.h>                             // for uint32_t
#include <string.h>                             // for memmove, size_t
#include <QTimer>                               // for QTimer
#include <algorithm>                            // for transform
#include <atomic>                               // for atomic
#include <memory>                               // for unique_ptr
#include <sstream>                              // for operator<<, stringstream

#include "OpenGLESDispatch/GLESv2Dispatch.h"    // for GLESv2Dispatch
#include "android/base/ArraySize.h"             // for base
#include "android/base/CpuUsage.h"              // for CpuUsage, CpuUsage::U...
#include "android/base/Log.h"                   // for LOG, LogMessage
#include "android/base/memory/MemoryTracker.h"  // for MemoryTracker, Memory...
#include "android/base/system/System.h"         // for System, System::MemUsage
#include "android/globals.h"                    // for android_hw
#include "android/skin/qt/gl-common.h"          // for createShader, CHECK_G...
#include "android/skin/qt/logging-category.h"   // for emu
#include "android/skin/qt/vector-text-font.h"   // for kVectorFontGlyphs

class QMouseEvent;
class QWheelEvent;
class QWidget;

using namespace android::base;

static void cpuUsage_onBeginCollection() {
    CpuUsage::get()->setMeasurementInterval(
            PerfStats3DWidget::kCollectionIntervalMs * 1000);
}

static void cpuUsage_onEndCollection() {
    CpuUsage::get()->setMeasurementInterval(
            CpuUsage::kDefaultMeasurementIntervalUs);
}

PerfStats3DWidget::PerfStats3DWidget(QWidget* parent)
    : GLWidget(parent) {
    setFocusPolicy(Qt::ClickFocus);

    connect(&mCollectionTimer, SIGNAL(timeout()),
            this, SLOT(getData()));

    mCollectionTimer.setInterval(kCollectionIntervalMs);

    // CPU usage stats

    int numGuestCores = 0;
    numGuestCores = android_hw->hw_cpu_ncore;

    float maxCpuPercentScale = 100.0f + 100.0f * numGuestCores;

    int currentGraphIndex = 0;

    mDataSources.emplace_back((DataSource){
        currentGraphIndex,
        {0.9f, 0.2f, 0.1f, 1.0f},
        "Total main loop + vCPUs",
        "%",
        0.0f,
        maxCpuPercentScale,
        [maxCpuPercentScale]() {
            DataSourcePoint res;
            res.val = 100.0f * CpuUsage::get()->getTotalMainLoopAndVcpuUsage();
            res.min = 0.0f;
            res.max = maxCpuPercentScale;
            return res;
        },
        cpuUsage_onBeginCollection,
        cpuUsage_onEndCollection,
    });

    mDataSources.emplace_back((DataSource){
        currentGraphIndex,
        {0.7f, 0.2f, 0.9f, 1.0f},
        "Main Loop",
        "%",
        0.0f,
        maxCpuPercentScale,
        [maxCpuPercentScale]() {
            DataSourcePoint res;
            res.val = 100.0f * CpuUsage::get()->getSingleAreaUsage(
                                       CpuUsage::UsageArea::MainLoop);
            res.min = 0.0f;
            res.max = maxCpuPercentScale;
            return res;
        },
        cpuUsage_onBeginCollection,
        cpuUsage_onEndCollection,
    });

    std::vector<Color> vCpuColors = {
        { 0.1f, 0.8f, 0.2f, 1.0f },
        { 0.1f, 0.6f, 0.8f, 1.0f },
        { 0.8f, 0.4f, 0.1f, 1.0f },
        { 0.8f, 0.1f, 0.4f, 1.0f },
        { 0.2f, 0.8f, 0.1f, 1.0f },
        { 0.2f, 0.4f, 0.8f, 1.0f },
        { 0.4f, 0.7f, 0.2f, 1.0f },
        { 0.7f, 0.2f, 0.5f, 1.0f },
        { 0.7f, 0.2f, 0.4f, 1.0f },
        { 0.5f, 0.7f, 0.2f, 1.0f },
        { 0.2f, 0.7f, 0.5f, 1.0f },
        { 0.0f, 0.7f, 0.4f, 1.0f },
    };

    for (int i = 0; i < numGuestCores; ++i) {
        std::stringstream ss;
        ss << "vCPU " << i;

        auto color = vCpuColors[i % vCpuColors.size()];

        mDataSources.emplace_back((DataSource){
            currentGraphIndex, vCpuColors[i % vCpuColors.size()], ss.str(),
            "%", 0.0f, maxCpuPercentScale,
            [i, maxCpuPercentScale]() {
                DataSourcePoint res;
                res.val = 100.0f *
                    CpuUsage::get()->getSingleAreaUsage(
                        CpuUsage::UsageArea::Vcpu + i);
                res.min = 0.0f;
                res.max = maxCpuPercentScale;
                return res;
            },
            cpuUsage_onBeginCollection,
            cpuUsage_onEndCollection,
        });
    }

    currentGraphIndex++;
    float minResMem = 0.0f;
    float maxResMem = 3.0 * android_hw->hw_ramSize;

    // Memory Usage Stats
    mDataSources.emplace_back(
            (DataSource){currentGraphIndex,
                         {0.7f, 0.2f, 1.0f, 1.0f},
                         "Resident Memory",
                         "MB",
                         minResMem,
                         maxResMem,
                         [minResMem, maxResMem]() mutable {
                             DataSourcePoint res;
                             auto usage = System::get()->getMemUsage();
                             res.val = usage.resident / 1048576.0f;
                             res.min = minResMem;
                             if (res.val > maxResMem) {
                                 maxResMem = res.val + 100;
                             }
                             res.max = maxResMem;
                             return res;
                         },
                         []() {},
                         []() {}});

    auto memTracker = MemoryTracker::get();
    if (memTracker && memTracker->isEnabled()) {
        mDataSources.emplace_back((DataSource){
                currentGraphIndex,
                {0.0f, 0.7f, 0.4f, 1.0f},
                "Emu GL Memory",
                "MB",
                minResMem,
                maxResMem,
                [memTracker, minResMem, maxResMem]() {
                    DataSourcePoint res;
                    res.val = memTracker->getUsage("EMUGL")->mLive.load() /
                              1048576.0f;
                    res.min = minResMem;
                    res.max = maxResMem;
                    return res;
                },
                []() {},
                []() {}});
    }
}

PerfStats3DWidget::~PerfStats3DWidget() {
    // Free up allocated resources.
    if (!mGLES2) {
        return;
    }
    if (readyForRendering()) {
        if(makeContextCurrent()) {
            mGLES2->glUseProgram(0);
            mGLES2->glDeleteProgram(mProgram);
            mGLES2->glDeleteBuffers(1, &mXAxisBuffer);
            mGLES2->glDeleteBuffers(1, &mFontBuffer);

            for (auto& dataSource : mDataSources) {
                mGLES2->glDeleteBuffers(1, &dataSource.valBuffer);
            }

            mDataSources.clear();
        }
    }
}

void PerfStats3DWidget::onCollectionEnabled() {
    for (auto& dataSource : mDataSources) {
        dataSource.onBeginCollection();
    }
    mCollectionTimer.start();
}

void PerfStats3DWidget::onCollectionDisabled() {
    mCollectionTimer.stop();
    for (auto& dataSource : mDataSources) {
        dataSource.onEndCollection();
    }
}

void PerfStats3DWidget::getData() {
    if (!mInitialized || !mGLES2) return;

    for (auto& dataSource: mDataSources) {
        if (dataSource.values.size() < kCollectionHistory) {
            dataSource.values.resize(kCollectionHistory);
        }

        auto pt = dataSource.getFunc();

        dataSource.min = pt.min;
        dataSource.max = pt.max;

        memmove(dataSource.values.data(),
                dataSource.values.data() + 1,
                sizeof(float) * (kCollectionHistory - 1));

        dataSource.values[kCollectionHistory - 1] = pt.val;

        auto gl = mGLES2;

        gl->glBindBuffer(GL_ARRAY_BUFFER, dataSource.valBuffer);
        gl->glBufferSubData(GL_ARRAY_BUFFER, 0,
                            dataSource.values.size() * sizeof(float),
                            dataSource.values.data());
        gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

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

    if (!initXAxisBuffer()) {
        LOG(ERROR) << "Could not initialize X axis buffer";
    }

    if (!initFontBuffer()) {
        LOG(ERROR) << "Could not initialize font buffer";
    }

    for (auto& dataSource: mDataSources) {
        initDataSourceBuffer(&dataSource.valBuffer);
    }

    resizeGL(width(), height());

    mInitialized = true;

    return true;
}

bool PerfStats3DWidget::initProgram() {
    auto gl = mGLES2;

    // Compile & link shaders.
    constexpr char vShaderSrc[] = R"(precision highp float;
attribute float xvalue;
attribute float yvalue;

uniform float xscale;
uniform float xoffset;
uniform float yscale;
uniform float yoffset;

void main() {
    gl_Position =
        vec4(xscale * xvalue + xoffset,
             yscale * yvalue + yoffset,
             0.0, 1.0);
})";

    constexpr char fShaderSrc[] = R"(precision highp float;
uniform vec4 color;
void main() {
    gl_FragColor = color;
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
        qCWarning(emu, "Failed to link program");
        return false;
    }

    CHECK_GL_ERROR_RETURN("Failed to initialize shaders", false);

    // We no longer need shader objects after successful program linkage.
    gl->glDetachShader(mProgram, vertex_shader);
    gl->glDetachShader(mProgram, fragment_shader);
    gl->glDeleteShader(vertex_shader);
    gl->glDeleteShader(fragment_shader);

    mXScaleUniformLoc = gl->glGetUniformLocation(mProgram, "xscale");
    mXOffsetUniformLoc = gl->glGetUniformLocation(mProgram, "xoffset");
    mYScaleUniformLoc = gl->glGetUniformLocation(mProgram, "yscale");
    mYOffsetUniformLoc = gl->glGetUniformLocation(mProgram, "yoffset");
    mColorUniformLoc = gl->glGetUniformLocation(mProgram, "color");

    return true;
}

bool PerfStats3DWidget::initXAxisBuffer() {
    auto gl = mGLES2;

    gl->glGenBuffers(1, &mXAxisBuffer);

    gl->glBindBuffer(GL_ARRAY_BUFFER, mXAxisBuffer);

    std::vector<float> xValues(kCollectionHistory);
    for (uint32_t i = 0; i < kCollectionHistory; ++i) {
        xValues[i] = 2.0f * float(i) / kCollectionHistory - 1.0f;
    }

    gl->glBufferData(GL_ARRAY_BUFFER, xValues.size() * sizeof(float),
                     xValues.data(), GL_STREAM_DRAW);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);

    CHECK_GL_ERROR_RETURN("Failed to initialize X axis buffer", false);

    return true;
}

bool PerfStats3DWidget::initFontBuffer() {
    auto gl = mGLES2;

    gl->glGenBuffers(1, &mFontBuffer);

    gl->glBindBuffer(GL_ARRAY_BUFFER, mFontBuffer);
    gl->glBufferData(GL_ARRAY_BUFFER, sizeof(kVectorFontGlyphs),
                     kVectorFontGlyphs, GL_STATIC_DRAW);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);

    CHECK_GL_ERROR_RETURN("Failed to initialize X axis buffer", false);

    return true;
}

void PerfStats3DWidget::drawText(const std::string& string,
                                 float x,
                                 float y,
                                 float height,
                                 const float color[4]) {
    std::string stringUpper(string);
    std::transform(string.begin(), string.end(), stringUpper.begin(), ::toupper);

    const char* chars = stringUpper.c_str();

    auto gl = mGLES2;

    gl->glBindBuffer(GL_ARRAY_BUFFER, mFontBuffer);
    gl->glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    gl->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                             (const GLvoid*)(sizeof(float)));

    float scaleY = height * 0.5f; // each glyph is really 2 units wide
    float scaleX = scaleY * 0.5f; // 1:2 aspect
    float currX = x + scaleX;
    float yOff = y + height * 0.5f;
    float dx = scaleY + scaleX * 0.3;

    constexpr GLint vertsPerStroke = 2;
    constexpr GLint vertsPerGlyph = STROKES_PER_GLYPH * vertsPerStroke;

    for (size_t i = 0 ; i < string.size(); ++i) {
        gl->glUniform1f(mXScaleUniformLoc, scaleX);
        gl->glUniform1f(mYScaleUniformLoc, scaleY);
        gl->glUniform1f(mXOffsetUniformLoc, currX);
        gl->glUniform1f(mYOffsetUniformLoc, yOff);
        gl->glUniform4fv(mColorUniformLoc, 1, color);
        GLint charVal = chars[i];
        gl->glDrawArrays(
            GL_LINES,
            vertsPerGlyph * charVal,
            vertsPerGlyph);
        currX += dx;
    }

    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

bool PerfStats3DWidget::initDataSourceBuffer(GLuint* buf) {
    auto gl = mGLES2;

    gl->glGenBuffers(1, buf);
    gl->glBindBuffer(GL_ARRAY_BUFFER, *buf);
    gl->glBufferData(GL_ARRAY_BUFFER, sizeof(float) * kCollectionHistory,
                     nullptr, GL_STREAM_DRAW);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);

    CHECK_GL_ERROR_RETURN("Failed to initialize data source buffer", false);

    return true;
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

    int totalAxes = 0;
    for (const auto& dataSource: mDataSources) {
        if (dataSource.axisIndex + 1 > totalAxes) {
            totalAxes = dataSource.axisIndex + 1;
        }
    }

    std::vector<std::vector<int>> subPlotIndices(totalAxes);
    std::vector<int> subPlotOffsets(mDataSources.size(), 0);

    for (int i = 0; i < totalAxes; ++i) {
        for (int j = 0; j < mDataSources.size(); ++j) {
            if (i == mDataSources[j].axisIndex) {
                subPlotIndices[i].push_back(j);
            }
        }
    }

    for (int i = 0; i < totalAxes; ++i) {
        int k = 0;
        for (int j = 0; j < subPlotIndices[i].size(); ++j) {
            subPlotOffsets[subPlotIndices[i][j]] = k;
            ++k;
        }
    }

    float numGraphs = static_cast<float>(totalAxes);

    for (int i = 0; i < mDataSources.size(); ++i) {
        const auto& dataSource = mDataSources[i];

        gl->glBindBuffer(GL_ARRAY_BUFFER, mXAxisBuffer);
        gl->glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);

        gl->glBindBuffer(GL_ARRAY_BUFFER, dataSource.valBuffer);
        gl->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float), 0);

        float scale = 2.0f;
        float range = dataSource.max - dataSource.min;
        if (range) scale = scale / range;

        float graphHeight = 1.0f / numGraphs;

        scale *= graphHeight;

        float offset = -1.0f;
        float axisIndexFloat = static_cast<float>(dataSource.axisIndex);
        offset += 2.0f * axisIndexFloat * graphHeight;

        gl->glUniform1f(mXScaleUniformLoc, 1.0f);
        gl->glUniform1f(mXOffsetUniformLoc, 0.0f);
        gl->glUniform1f(mYScaleUniformLoc, scale);
        gl->glUniform1f(mYOffsetUniformLoc, offset);
        gl->glUniform4fv(mColorUniformLoc, 1, (const GLfloat*)(&dataSource.color));
        gl->glDrawArrays(GL_LINE_STRIP, 0, kCollectionHistory);

        if (dataSource.values.size() >= kCollectionHistory) {
            float textHeight = 0.06f;

            int subPlotOffset = subPlotOffsets[i];
            int totalSubPlots = subPlotIndices[dataSource.axisIndex].size();

            if (graphHeight < totalSubPlots * textHeight) {
                textHeight = graphHeight / totalSubPlots;
            }

            std::stringstream ss;
            ss << dataSource.name;
            ss << " " << dataSource.values[kCollectionHistory - 1] << " "
               << dataSource.unit;
            drawText(ss.str(),
                     -1.0f, // left edge,
                     // vertical offset from graph top according to subplot offset
                     offset + 2.0f * graphHeight -
                             textHeight * (1.0f + subPlotOffset),
                     textHeight, (const float*)(&dataSource.color));
        }
    }

    gl->glDisableVertexAttribArray(0);
    gl->glDisableVertexAttribArray(1);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// TODO: Scrolling
void PerfStats3DWidget::mouseMoveEvent(QMouseEvent* event) {
}

void PerfStats3DWidget::wheelEvent(QWheelEvent* event) {
}

void PerfStats3DWidget::mousePressEvent(QMouseEvent* event) {
}

void PerfStats3DWidget::mouseReleaseEvent(QMouseEvent* event) {
}
