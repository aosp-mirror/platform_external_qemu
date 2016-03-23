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
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QVector3D>
#include <QSurfaceFormat>
#include <string>

// A widget that displays a 3D model of a device and lets the user
// manipulate its rotation. The changes in rotation can be used to
// derive the values which should be reported by the virtual accelerometer
// and magnetometer.
class Accelerometer3DWidget : public GLWidget {
    Q_OBJECT
    Q_PROPERTY(QQuaternion rotation READ rotation WRITE setRotation NOTIFY rotationChanged USER true);

public:
     Accelerometer3DWidget(QWidget *parent = 0);
    ~Accelerometer3DWidget();

signals:
    // Emitted when the model's rotation changes.
    void rotationChanged(const QQuaternion&);

public slots:
    // Sets the rotation quaternion.
    void setRotation(const QQuaternion& quat) { mQuat = quat; }

public:
    // Returns the rotation quaternion.
    const QQuaternion& rotation() const { return mQuat; }

protected:
    // This is called once, after the GL context is created, to do some one-off
    // setup work.
    bool initGL() override;

    // Called every time the widget changes its dimensions.
    void resizeGL(int, int) override;

    // Called every time the widget needs to be repainted.
    void repaintGL() override;

    // This is called every time the mouse is moved.
    void mouseMoveEvent(QMouseEvent *event);

    void mousePressEvent(QMouseEvent *event) {
        mPrevMouseX = event->x();
        mPrevMouseY = event->y();
        mTracking = true;
    }
    void mouseReleaseEvent(QMouseEvent*) { mTracking = false; }

private:
    bool initProgram();
    bool initModel();
    bool initTextures();

    QQuaternion mQuat;
    QMatrix4x4 mPerspective;
    QMatrix4x4 mCameraTransform;

    GLuint mProgram;
    GLuint mVertexDataBuffer;
    GLuint mVertexIndexBuffer;
    GLuint mDiffuseMap;
    GLuint mSpecularMap;
    GLuint mGlossMap;
    GLuint mEnvMap;
    int mElementsCount;

    GLuint mVertexPositionAttribute;
    GLuint mVertexNormalAttribute;
    GLuint mVertexUVAttribute;

    int mPrevMouseX;
    int mPrevMouseY;
    bool mTracking;
};
