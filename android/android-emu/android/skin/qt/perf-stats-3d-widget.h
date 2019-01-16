// Copyright 2016 The Android Open Source Project
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/skin/qt/gl-widget.h"
#include "GLES2/gl2.h"

#include <QMouseEvent>

#include <array>
#include <string>

// Widget for graphing emulator performance stats over time.
class PerfStats3DWidget : public GLWidget {
    Q_OBJECT
public:
     PerfStats3DWidget(QWidget *parent = 0);
    ~PerfStats3DWidget();

    void updateAndRefreshValues();

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
    bool initGraphLines();

    void refreshData();

    GLuint mProgram;
    GLuint mGraphDataBuffer;
    int mCollectionHistory = 360;

    std::vector<float> mData;
    struct GraphPoint {
        float x;
        float y;
    };
    std::vector<GraphPoint> mGraphPoints;

    // TODO
    float mLineThickness = 1.0f;
    bool mDataAvailable = false;

    GLuint mVertexPositionAttribute;
};
