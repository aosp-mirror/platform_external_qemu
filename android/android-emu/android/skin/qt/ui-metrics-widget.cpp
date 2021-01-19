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

#include "android/skin/qt/ui-metrics-widget.h"

#include <ctype.h>             // for toupper
#include <qloggingcategory.h>  // for qCWarning
#include <qnamespace.h>        // for ClickFocus
#include <stdint.h>            // for uint32_t
#include <string.h>            // for memmove, size_t
#include <QTimer>              // for QTimer
#include <algorithm>           // for transform
#include <atomic>              // for atomic
#include <memory>              // for unique_ptr
#include <sstream>             // for operator<<, stringstream

#include "OpenGLESDispatch/GLESv2Dispatch.h"  // for GLESv2Dispatch
#include "android/base/ArraySize.h"           // for base
#include "android/base/Log.h"                 // for LOG, LogMessage
#include "android/base/async/ThreadLooper.h"
#include "android/base/system/System.h"        // for System, System::MemUsage
#include "android/globals.h"                   // for android_hw
#include "android/skin/qt/gl-common.h"         // for createShader, CHECK_G...
#include "android/skin/qt/logging-category.h"  // for emu
#include "android/skin/qt/vector-text-font.h"  // for kVectorFontGlyphs
#include "android/skin/winsys.h"

#if defined(__APPLE__)
#include "android/skin/qt/mac-native-window.h"
#elif defined(_WIN32)
#include "android/skin/qt/windows-native-window.h"
#else
#endif
class QWidget;

using namespace android::base;

struct FrameDataPoint {
    float timeInterval = 0.0f;
    int frameIdx = -1;
    uint64_t timeStamp = 0;
};

struct GlobalData {
    void clear() {
        mDataStore.clear();
        mPrevDataPoint = {0.0f, -1, 0};
    }
    std::vector<FrameDataPoint> mDataStore;
    FrameDataPoint mPrevDataPoint;
};

static GlobalData* globalData() {
    static GlobalData sGlobal;
    return &sGlobal;
}

static int sX = 0;
static int sY = 0;
static const int kBytesPerPixel = 4;
static int sBytesPerRow = 0;

UiMetricsWidget::UiMetricsWidget(QWidget* parent)
    : GLWidget(parent),
      mScreenCapTask(
              android::base::ThreadLooper::get(),
              [this]() { return winGetData(); },
              kScreenCapIntervalMs) {
    setFocusPolicy(Qt::ClickFocus);
    // Query screen width and height
    SkinRect monitor;
    skin_winsys_get_monitor_rect(&monitor);
    mScreenWidth = monitor.size.w;
    mScreenHeight = monitor.size.h;
    sBytesPerRow = kBytesPerPixel * mScreenWidth;

    connect(&mCollectionTimer, SIGNAL(timeout()), this, SLOT(getData()));

    mCollectionTimer.setInterval(kCollectionIntervalMs);
    int currentGraphIndex = 0;
    // frame interval variance
    mDataSources.emplace_back((DataSource){currentGraphIndex,
                                           {0.7f, 0.2f, 1.0f, 1.0f},
                                           "Frame time interval",
                                           "ms",
                                           kMinTime,
                                           kMaxTime});
#if defined(_WIN32)
    mPixelData.resize(mScreenWidth * mScreenHeight * kBytesPerPixel, 0);
#endif
}

UiMetricsWidget::~UiMetricsWidget() {
    // Free up allocated resources.
    if (!mGLES2) {
        return;
    }
    if (readyForRendering()) {
        if (makeContextCurrent()) {
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

static void onFrameAvailableCallback(unsigned char* pixel, uint64_t timeStamp) {
    int32_t r = pixel[sY * sBytesPerRow + sX * kBytesPerPixel];
    int32_t g = pixel[sY * sBytesPerRow + sX * kBytesPerPixel + 1];
    int32_t b = pixel[sY * sBytesPerRow + sX * kBytesPerPixel + 2];
    int frame = r + (g << 8) + (b << 16);

    FrameDataPoint pt;
    pt.timeInterval = 0;
    pt.timeStamp = timeStamp;
    pt.frameIdx = frame;
    // first measurement point
    if (globalData()->mPrevDataPoint.frameIdx != -1) {
        if (globalData()->mPrevDataPoint.frameIdx >= frame) {
            return;
        }
        pt.timeInterval =
                (float)((timeStamp - globalData()->mPrevDataPoint.timeStamp) /
                        1000000.0f);
        globalData()->mDataStore.push_back(pt);
    }
    globalData()->mPrevDataPoint = pt;
}

bool UiMetricsWidget::winGetData() {
#if defined(_WIN32)
    bool res = takeScreenshot(mPixelData.data(), mScreenWidth, mScreenHeight);
    if (!res)
        return;
    const auto now = System::get()->getHighResTimeUs();
    onFrameAvailableCallback(mPixelData.data(), now);
#endif
    return true;
}

// Actively poll for screen contents and extract frame number from pixel value.
void UiMetricsWidget::startScreenCapture(int x, int y) {
    sX = x;
    sY = y;
#ifdef __APPLE__
    startDisplayStream(&onFrameAvailableCallback);
#elif defined(_WIN32)
    mScreenCapTask.start();
#else
#endif
}

void UiMetricsWidget::stopScreenCapture() {
#ifdef __APPLE__
    stopDisplayStream();
#elif defined(_WIN32)
    mScreenCapTask.stopAsync();
#else
#endif
}

void UiMetricsWidget::onCollectionEnabled(int x, int y) {
    startScreenCapture(x, y);
    mCollectionTimer.start();
}

void UiMetricsWidget::onCollectionDisabled() {
    globalData()->clear();
    stopScreenCapture();
    mCollectionTimer.stop();
}

void UiMetricsWidget::getData() {
    if (!mInitialized || !mGLES2) {
        return;
    }
    for (auto& dataSource : mDataSources) {
        if (dataSource.values.size() < kCollectionHistory) {
            dataSource.values.resize(kCollectionHistory, 0.0);
        }

        if (globalData()->mDataStore.empty())
            continue;

        int origSize = globalData()->mDataStore.size();
        int totalCopy = std::min(kCollectionHistory, origSize);
        if (origSize < kCollectionHistory) {
            memmove(dataSource.values.data(),
                    dataSource.values.data() + origSize,
                    sizeof(float) * (kCollectionHistory - origSize));
        }
        for (int i = totalCopy; i > 0; i--) {
            dataSource.values[kCollectionHistory - i] =
                    globalData()->mDataStore[i - 1].timeInterval;
        }
        globalData()->mDataStore.erase(
                globalData()->mDataStore.begin(),
                globalData()->mDataStore.begin() + totalCopy);

        auto gl = mGLES2;

        gl->glBindBuffer(GL_ARRAY_BUFFER, dataSource.valBuffer);
        gl->glBufferSubData(GL_ARRAY_BUFFER, 0,
                            dataSource.values.size() * sizeof(float),
                            dataSource.values.data());
        gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    renderFrame();
}

bool UiMetricsWidget::initGL() {
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

    for (auto& dataSource : mDataSources) {
        initDataSourceBuffer(&dataSource.valBuffer);
    }
    resizeGL(width(), height());

    mInitialized = true;

    return true;
}

bool UiMetricsWidget::initProgram() {
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

    GLuint vertex_shader = createShader(gl, GL_VERTEX_SHADER, vShaderSrc);
    GLuint fragment_shader = createShader(gl, GL_FRAGMENT_SHADER, fShaderSrc);

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

bool UiMetricsWidget::initXAxisBuffer() {
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

bool UiMetricsWidget::initFontBuffer() {
    auto gl = mGLES2;

    gl->glGenBuffers(1, &mFontBuffer);

    gl->glBindBuffer(GL_ARRAY_BUFFER, mFontBuffer);
    gl->glBufferData(GL_ARRAY_BUFFER, sizeof(kVectorFontGlyphs),
                     kVectorFontGlyphs, GL_STATIC_DRAW);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);

    CHECK_GL_ERROR_RETURN("Failed to initialize X axis buffer", false);

    return true;
}

void UiMetricsWidget::drawText(const std::string& string,
                               float x,
                               float y,
                               float height,
                               const float color[4]) {
    std::string stringUpper(string);
    std::transform(string.begin(), string.end(), stringUpper.begin(),
                   ::toupper);

    const char* chars = stringUpper.c_str();

    auto gl = mGLES2;

    gl->glBindBuffer(GL_ARRAY_BUFFER, mFontBuffer);
    gl->glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 2 * sizeof(float), 0);
    gl->glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 2 * sizeof(float),
                              (const GLvoid*)(sizeof(float)));

    float scaleY = height * 0.5f;  // each glyph is really 2 units wide
    float scaleX = scaleY * 0.5f;  // 1:2 aspect
    float currX = x + scaleX;
    float yOff = y + height * 0.5f;
    float dx = scaleY + scaleX * 0.3;

    constexpr GLint vertsPerStroke = 2;
    constexpr GLint vertsPerGlyph = STROKES_PER_GLYPH * vertsPerStroke;

    for (size_t i = 0; i < string.size(); ++i) {
        gl->glUniform1f(mXScaleUniformLoc, scaleX);
        gl->glUniform1f(mYScaleUniformLoc, scaleY);
        gl->glUniform1f(mXOffsetUniformLoc, currX);
        gl->glUniform1f(mYOffsetUniformLoc, yOff);
        gl->glUniform4fv(mColorUniformLoc, 1, color);
        GLint charVal = chars[i];
        gl->glDrawArrays(GL_LINES, vertsPerGlyph * charVal, vertsPerGlyph);
        currX += dx;
    }

    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
}

bool UiMetricsWidget::initDataSourceBuffer(GLuint* buf) {
    auto gl = mGLES2;

    gl->glGenBuffers(1, buf);
    gl->glBindBuffer(GL_ARRAY_BUFFER, *buf);
    gl->glBufferData(GL_ARRAY_BUFFER, sizeof(float) * kCollectionHistory,
                     nullptr, GL_STREAM_DRAW);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);

    CHECK_GL_ERROR_RETURN("Failed to initialize data source buffer", false);

    return true;
}

void UiMetricsWidget::resizeGL(int w, int h) {
    if (!mGLES2) {
        return;
    }

    // Adjust for new viewport
    mGLES2->glViewport(0, 0, w, h);
}

void UiMetricsWidget::repaintGL() {
    if (!mGLES2) {
        return;
    }

    auto gl = mGLES2;

    gl->glUseProgram(mProgram);

    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    gl->glEnableVertexAttribArray(0);
    gl->glEnableVertexAttribArray(1);

    int totalAxes = 0;
    for (const auto& dataSource : mDataSources) {
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
        if (range)
            scale = scale / range;

        float graphHeight = 1.0f / numGraphs;

        scale *= graphHeight;

        float offset = -1.0f;
        float axisIndexFloat = static_cast<float>(dataSource.axisIndex);
        offset += 2.0f * axisIndexFloat * graphHeight;

        gl->glUniform1f(mXScaleUniformLoc, 1.0f);
        gl->glUniform1f(mXOffsetUniformLoc, 0.0f);
        gl->glUniform1f(mYScaleUniformLoc, scale);
        gl->glUniform1f(mYOffsetUniformLoc, offset);
        gl->glUniform4fv(mColorUniformLoc, 1,
                         (const GLfloat*)(&dataSource.color));
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
                     -1.0f,  // left edge,
                     // vertical offset from graph top according to subplot
                     // offset
                     offset + 2.0f * graphHeight -
                             textHeight * (1.0f + subPlotOffset) - 0.1f,
                     textHeight, (const float*)(&dataSource.color));
        }
    }

    gl->glDisableVertexAttribArray(0);
    gl->glDisableVertexAttribArray(1);
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
}
