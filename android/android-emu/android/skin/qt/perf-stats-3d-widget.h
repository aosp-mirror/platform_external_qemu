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

#pragma once

#include "android/skin/qt/gl-widget.h"
#include "GLES2/gl2.h"

#include <QMouseEvent>
#include <QTimer>

#include <array>
#include <functional>
#include <string>

// Widget for graphing emulator performance stats over time.
class PerfStats3DWidget : public GLWidget {
    Q_OBJECT
public:
    // 5 Hz, 60 seconds
    static constexpr int kCollectionIntervalMs = 200;
    static constexpr int kCollectionHistory = 300;

     PerfStats3DWidget(QWidget *parent = 0);
    ~PerfStats3DWidget();

    void onCollectionEnabled();
    void onCollectionDisabled();

public slots:
    void getData();

protected:
    // This is called once, after the GL context is created, to do some one-off
    // setup work.
    bool initGL() override;

    // Called every time the widget changes its dimensions.
    void resizeGL(int, int) override;

    // Called every time the widget needs to be repainted.
    void repaintGL() override;

    // This is called every time the mouse is moved.
    void mouseMoveEvent(QMouseEvent* event) override;
    // This is called every time the mouse wheel is moved.
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool initProgram();
    bool initXAxisBuffer();
    bool initFontBuffer();
    bool initDataSourceBuffer(GLuint* buf);

    void drawText(const std::string& string,
                  float x, float y,
                  float height,
                  const float color[4]);

    struct DataSourcePoint {
        float val = 0.0f;
        float min = 0.0f;
        float max = 0.0f;
    };

    using DataSourceGetter =
            std::function<DataSourcePoint()>;
    using DataSourceControlCallback =
            std::function<void()>;

    QTimer mCollectionTimer;

    GLuint mProgram = 0;
    GLint mXScaleUniformLoc = -1;
    GLint mXOffsetUniformLoc = -1;
    GLint mYScaleUniformLoc = -1;
    GLint mYOffsetUniformLoc = -1;
    GLint mColorUniformLoc = -1;

    GLuint mXAxisBuffer = 0;
    GLuint mFontBuffer = 0;

    struct Color {
        float r;
        float g;
        float b;
        float a;
    };

    struct DataSource {
        int axisIndex;
        Color color;

        std::string name;
        std::string unit;

        float min;
        float max;

        DataSourceGetter getFunc;
        DataSourceControlCallback onBeginCollection;
        DataSourceControlCallback onEndCollection;

        std::vector<float> values;

        // For drawing:
        GLuint valBuffer;
        GLuint axisBorderBuffer;
    };

    std::vector<DataSource> mDataSources;

    // TODO
    bool mInitialized = false;
};