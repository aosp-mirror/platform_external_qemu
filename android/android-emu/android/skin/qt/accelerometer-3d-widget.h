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

#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <string>

struct QAndroidSensorsAgent;

// A widget that displays a 3D model of a device and lets the user
// manipulate its rotation and position.
// The changes in rotation and position can be used to
// derive the values which should be reported by the virtual accelerometer,
// gyroscope, and magnetometer.
class Accelerometer3DWidget : public GLWidget {
    Q_OBJECT

    Q_PROPERTY(glm::quat targetRotation READ targetRotation WRITE setTargetRotation NOTIFY targetRotationChanged USER true);
    Q_PROPERTY(glm::vec3 targetPosition READ targetPosition WRITE setTargetPosition NOTIFY targetPositionChanged USER true);
public:
    enum class OperationMode {
        Rotate,
        Move
    };

     Accelerometer3DWidget(QWidget *parent = 0);
    ~Accelerometer3DWidget();

    void setSensorsAgent(const QAndroidSensorsAgent* agent);

signals:
    // Emitted when the model's rotation changes.
    void targetRotationChanged();

    // Emitted when the position of the model changes.
    void targetPositionChanged();

    // Emitted when the user begins dragging the model.
    void dragStarted();

    // Emitted when the usr stops dragging the model.
    void dragStopped();

public slots:
    // Sets the target rotation.
    void setTargetRotation(const glm::quat& rotation);

    // Sets the X, Y, and Z coordinates of the model's origin.
    void setTargetPosition(const glm::vec3& position);

    // Sets the widget's operation mode which determines how it reacts
    // to mose dragging. It may either rotate the model or move it around.
    void setOperationMode(OperationMode mode) { mOperationMode = mode; }

public:
    // Getters for the rotation quaternion and delta.
    const glm::quat& targetRotation() const { return mTargetRotation; }

    // Returns the X, Y and Z coordinates of the model's origin.
    const glm::vec3& targetPosition() const { return mTargetPosition; }

    static constexpr float MaxX = 7.f;
    static constexpr float MinX = -7.f;
    static constexpr float MaxY = 4.f;
    static constexpr float MinY = -4.f;
    static constexpr float MaxZ = 4.f;
    static constexpr float MinZ = -4.f;
    static constexpr float CameraDistance = 40.f;
    static constexpr float NearClip = 1.f;
    static constexpr float FarClip = 100.f;
    // Each 15 degree wheel movement should move 0.2 of an inch.
    static constexpr float InchesPerWheelDegree = 2.f / 150.f;

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
    bool initModel();
    bool initTextures();

    // Returns the world coordinates of a point on the XY plane
    // that corresponds to the given point on the screen.
    glm::vec3 screenToWorldCoordinate(int x, int y) const;

    const QAndroidSensorsAgent* mSensorsAgent = nullptr;

    glm::quat mTargetRotation = glm::quat();
    glm::vec3 mTargetPosition = glm::vec3();

    glm::quat mLastTargetRotation = glm::quat();
    glm::vec3 mLastTargetPosition = glm::vec3();

    glm::mat4 mPerspective;
    glm::mat4 mPerspectiveInverse;
    glm::mat4 mCameraTransform;
    glm::mat4 mCameraTransformInverse;

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

    int mPrevMouseX = 0;
    int mPrevMouseY = 0;
    glm::vec3 mPrevDragOrigin;
    bool mTracking = false;
    OperationMode mOperationMode = OperationMode::Rotate;
};
