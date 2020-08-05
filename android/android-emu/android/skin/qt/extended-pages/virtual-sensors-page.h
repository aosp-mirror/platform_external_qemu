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

#include <qobjectdefs.h>                           // for slots, Q_OBJECT
#include <QDoubleValidator>                        // for QDoubleValidator
#include <QElapsedTimer>                           // for QElapsedTimer
#include <QString>                                 // for QString
#include <QTimer>                                  // for QTimer
#include <QWidget>                                 // for QWidget
#include <memory>                                  // for unique_ptr

#include "android/hw-sensors.h"                    // for PhysicalParameter
#include "android/metrics/PeriodicReporter.h"      // for PeriodicReporter
#include "android/physics/Physics.h"               // for PhysicalInterpolation
#include "android/physics/physical_state_agent.h"  // for QAndroidPhysicalSt...
#include "android/skin/rect.h"                     // for SkinRotation, SKIN...
#include "glm/detail/../fwd.hpp"                   // for quat
#include "glm/detail/type_vec.hpp"                 // for vec3
#include "glm/detail/type_vec3.hpp"                // for tvec3

class QObject;
class QShowEvent;
class QWidget;
struct QAndroidSensorsAgent;

class VirtualSensorsPage : public QWidget
{
    Q_OBJECT

public:
    explicit VirtualSensorsPage(QWidget* parent = 0);
    ~VirtualSensorsPage();

    void showEvent(QShowEvent* event) override;
    static void setSensorsAgent(const QAndroidSensorsAgent* agent);
    void hideRotationButtons(bool hide);

private slots:
    void on_temperatureSensorValueWidget_valueChanged(double value);
    void on_proximitySensorValueWidget_valueChanged(double value);
    void on_lightSensorValueWidget_valueChanged(double value);
    void on_pressureSensorValueWidget_valueChanged(double value);
    void on_humiditySensorValueWidget_valueChanged(double value);
    void on_accelModeRotate_toggled();
    void on_accelModeMove_toggled();
    void on_accelModeFold_toggled();
    void on_accelModeRoll_toggled();

    void on_magNorthWidget_valueChanged(double value);
    void on_magEastWidget_valueChanged(double value);
    void on_magVerticalWidget_valueChanged(double value);

    void updateTargetState();

    void propagateAccelWidgetChange();
    void propagateSlidersChange();

    void updateModelFromAccelWidget(PhysicalInterpolation mode);
    void updateModelFromSliders(PhysicalInterpolation mode);
    void updateUIFromModelCurrentState();

    void updateSensorValuesInUI();

signals:
    void coarseOrientationChanged(SkinRotation);
    void updateResultingValuesRequired(glm::vec3 acceleration,
                                       glm::vec3 gyroscope,
                                       glm::vec3 device_magnetic_vector);
    void updateTargetStateRequired();
    void startSensorUpdateTimerRequired();
    void stopSensorUpdateTimerRequired();
    void windowVisible();
    void virtualSensorsInteraction();

public slots:
    void onVirtualSceneControlsEngaged(bool engaged);
    // Sets the device orientation to: face up, pointing in the
    // desired direction (north = 0.0, east = 90.0)
    void setTargetHeadingDegrees(double heading);

private slots:
    void on_rotateToPortrait_clicked();
    void on_rotateToLandscape_clicked();
    void on_rotateToReversePortrait_clicked();
    void on_rotateToReverseLandscape_clicked();
    void on_btn_postureClosed_clicked();
    void on_btn_postureFlipped_clicked();
    void on_btn_postureHalfOpen_clicked();
    void on_btn_postureOpen_clicked();
    void on_btn_postureTent_clicked();
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
    void on_positionZSlider_valueChanged(double);
    void on_hinge0Slider_valueChanged(double);
    void on_hinge1Slider_valueChanged(double);
    void on_hinge2Slider_valueChanged(double);
    void on_roll0Slider_valueChanged(double);
    void on_roll1Slider_valueChanged(double);
    void on_roll2Slider_valueChanged(double);
    void on_posture_valueChanged(int);

    void updateResultingValues(glm::vec3 acceleration,
                               glm::vec3 gyroscope,
                               glm::vec3 device_magnetic_vector);

    void startSensorUpdateTimer();
    void stopSensorUpdateTimer();
private:
    static const QAndroidSensorsAgent* sSensorsAgent;

    void reportVirtualSensorsInteraction();

    void resetDeviceRotation(const glm::quat&);

    float getPhysicalParameterTarget(PhysicalParameter parameter_id);
    glm::vec3 getPhysicalParameterTargetVec3(PhysicalParameter parameter_id);
    void setPhysicalParameterTarget(PhysicalParameter parameter_id,
            PhysicalInterpolation mode,
            double v1,
            double v2 = 0.0,
            double v3 = 0.0);
    void setCoarseOrientation(AndroidCoarseOrientation orientation);

    void onTargetStateChanged();
    void onPhysicalStateChanging();
    void onPhysicalStateStabilized();
    void setupHingeSensorUI();
    void togglePostureButtonsVisibility(bool newVisibility);
    void setupRollableUI();
    void updateUIPosture();

    static void onTargetStateChanged(void* context);
    static void onPhysicalStateChanging(void* context);
    static void onPhysicalStateStabilized(void* context);

    std::unique_ptr<Ui::VirtualSensorsPage> mUi;
    QDoubleValidator mMagFieldValidator;
    QTimer mAccelerationTimer;
    bool mFirstShow = true;
    SkinRotation mCoarseOrientation = SKIN_ROTATION_0;
    bool mVirtualSensorsUsed = false;
    android::metrics::PeriodicReporter::TaskToken mMetricsReportingToken;

    QAndroidPhysicalStateAgent mQAndroidPhysicalStateAgent;
    bool mIsUIModifyingPhysicalState = false;

    bool mSlidersUseCurrent = true;
    glm::vec3 mSlidersTargetPosition;
    glm::vec3 mSlidersTargetRotation;

    bool mBypassOrientationChecks = false;
    bool mVirtualSceneControlsEngaged = false;
    QElapsedTimer mLastInteractionElapsed;
    enum FoldablePostures mCurrentPosture = POSTURE_UNKNOWN;

    void updateCurrentPosture();
};
