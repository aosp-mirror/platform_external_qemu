/* Copyright (C) 2020 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

#pragma once

#include <qobjectdefs.h>  // for Q_OBJECT, slots
#include <QString>        // for QString
#include <QTimer>         // for QTimer
#include <functional>     // for function
#include <string>         // for string
#include <vector>         // for vector

#include "GLES3/gl3.h"  // for GLuint, GLint
#include "android/base/async/RecurrentTask.h"
#include "android/skin/qt/gl-widget.h"

// Widget for graphing separate process emulator
// rendering performance stats over time.
class UiMetricsWidget : public GLWidget {
    Q_OBJECT
public:
    // 5 Hz, 60 seconds
    static constexpr int kCollectionIntervalMs = 200;  // ms
    static constexpr int kCollectionHistory = 300;
    static constexpr int kMinTime = 10;              // ms
    static constexpr int kMaxTime = 300;             // ms
    static constexpr int kScreenCapIntervalMs = 15;  // ms
    UiMetricsWidget(QWidget* parent = 0);
    ~UiMetricsWidget();

    void onCollectionEnabled(int x, int y);
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

private:
    bool initProgram();
    bool initXAxisBuffer();
    bool initFontBuffer();
    bool initDataSourceBuffer(GLuint* buf);
    bool winGetData();
    void startScreenCapture(int x, int y);
    void stopScreenCapture();
    void drawText(const std::string& string,
                  float x,
                  float y,
                  float height,
                  const float color[4]);

    struct DataSourcePoint {
        float val = 0.0f;
        float min = 0.0f;
        float max = 0.0f;
    };

    using DataSourceGetter = std::function<std::vector<DataSourcePoint>()>;
    using DataSourceControlCallback = std::function<void()>;
    int mScreenWidth;
    int mScreenHeight;
    android::base::RecurrentTask mScreenCapTask;
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
        std::vector<float> values;

        // For drawing:
        GLuint valBuffer;
        GLuint axisBorderBuffer;
    };

    std::vector<DataSource> mDataSources;
    std::vector<unsigned char> mPixelData;

    // TODO
    bool mInitialized = false;
};
