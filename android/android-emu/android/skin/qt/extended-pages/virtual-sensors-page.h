// Copyright (C) 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
#pragma once

#include "ui_virtual-sensors-page.h"

#include "android/hw-sensors.h"
#include "android/metrics/PeriodicReporter.h"
#include "android/physics/Physics.h"
#include "android/physics/physical_state_agent.h"
#include "android/skin/file.h"
#include "android/skin/rect.h"

#include <QDoubleValidator>
#include <QTimer>
#include <QWidget>

#include <memory>

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

struct QAndroidSensorsAgent;
class VirtualSensorsPage : public QWidget
{
    Q_OBJECT

public:
    explicit VirtualSensorsPage(QWidget* parent = 0);
    ~VirtualSensorsPage();

    void setSensorsAgent(const QAndroidSensorsAgent* agent);

private slots:
    void on_temperatureSensorValueWidget_valueChanged(double value);
    void on_proximitySensorValueWidget_valueChanged(double value);
    void on_lightSensorValueWidget_valueChanged(double value);
    void on_pressureSensorValueWidget_valueChanged(double value);
    void on_humiditySensorValueWidget_valueChanged(double value);
    void on_accelModeRotate_toggled();
    void on_accelModeMove_toggled();

    void on_magNorthWidget_valueChanged(double value);
    void on_magEastWidget_valueChanged(double value);
    void on_magVerticalWidget_valueChanged(double value);

    void propagateAccelWidgetChange();
    void propagateSlidersChange();

    void updateAccelWidgetFromSliders();
    void updateSlidersFromAccelWidget();
    void updateModelFromAccelWidget(PhysicalInterpolation mode);
    void updateAccelWidgetAndSlidersFromModel();

    void updateSensorValuesInUI();

    void onDragStarted();
    void onDragStopped();

    void onSkinLayoutChange(SkinRotation rot);

signals:
    void coarseOrientationChanged(SkinRotation);
    void updateResultingValuesRequired(glm::vec3 acceleration,
                                       glm::vec3 gyroscope,
                                       glm::vec3 device_magnetic_vector);
    void startSensorUpdateTimerRequired();
    void stopSensorUpdateTimerRequired();

private slots:
    void on_rotateToPortrait_clicked();
    void on_rotateToLandscape_clicked();
    void on_rotateToReversePortrait_clicked();
    void on_rotateToReverseLandscape_clicked();
    void on_helpMagneticField_clicked();
    void on_helpLight_clicked();
    void on_helpPressure_clicked();
    void on_helpAmbientTemp_clicked();
    void on_helpProximity_clicked();
    void on_helpHumidity_clicked();
    void on_zRotSlider_valueChanged(double);
    void on_xRotSlider_valueChanged(double);
    void on_yRotSlider_valueChanged(double);
    void on_positionXSlider_valueChanged(double);
    void on_positionYSlider_valueChanged(double);

    void updateResultingValues(glm::vec3 acceleration,
                               glm::vec3 gyroscope,
                               glm::vec3 device_magnetic_vector);

    void startSensorUpdateTimer();
    void stopSensorUpdateTimer();

private:
    void showEvent(QShowEvent*) override;

    void resetDeviceRotation(const glm::quat&);
    void resetDeviceRotationFromSkinLayout(SkinRotation orientation);

    void setPhysicalParameterTarget(PhysicalParameter parameter_id,
            PhysicalInterpolation mode,
            double v1,
            double v2 = 0.0,
            double v3 = 0.0);

    void onTargetStateChanged();
    void onPhysicalStateChanging();
    void onPhysicalStateStabilized();

    static void onTargetStateChanged(void* context);
    static void onPhysicalStateChanging(void* context);
    static void onPhysicalStateStabilized(void* context);

    std::unique_ptr<Ui::VirtualSensorsPage> mUi;
    QDoubleValidator mMagFieldValidator;
    const QAndroidSensorsAgent* mSensorsAgent;
    QTimer mAccelerationTimer;
    bool mIsDragging = false;
    bool mFirstShow = true;
    SkinRotation mCoarseOrientation = SKIN_ROTATION_0;
    bool mVirtualSensorsUsed = false;
    android::metrics::PeriodicReporter::TaskToken mMetricsReportingToken;

    QAndroidPhysicalStateAgent mQAndroidPhysicalStateAgent;
    bool mIsUIModifyingPhysicalState = false;
    bool mIsUpdatingUIFromModel = false;
};
