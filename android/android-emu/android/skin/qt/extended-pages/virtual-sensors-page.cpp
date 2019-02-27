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
#include "android/skin/qt/extended-pages/virtual-sensors-page.h"

#include "android/emulation/control/sensors_agent.h"
#include "android/hw-sensors.h"
#include "android/metrics/PeriodicReporter.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/physics/GlmHelpers.h"
#include "android/skin/ui.h"

#include "android/skin/qt/emulator-qt-window.h"
#include "android/skin/qt/stylesheet.h"

#include <QDesktopServices>
#include <QTextStream>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <cassert>

constexpr float kMetersPerInch = 0.0254f;
constexpr uint64_t kMinInteractionTimeMilliseconds = 500;

int kAccelerometerTabIndex = 0;

const QAndroidSensorsAgent* VirtualSensorsPage::sSensorsAgent = nullptr;

VirtualSensorsPage::VirtualSensorsPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::VirtualSensorsPage()) {
    mQAndroidPhysicalStateAgent.onTargetStateChanged = onTargetStateChanged;
    mQAndroidPhysicalStateAgent.onPhysicalStateChanging =
            onPhysicalStateChanging;
    mQAndroidPhysicalStateAgent.onPhysicalStateStabilized =
            onPhysicalStateStabilized;
    mQAndroidPhysicalStateAgent.context = reinterpret_cast<void*>(this);

    mUi->setupUi(this);

    // Set up ranges for UI sliders.
    mUi->temperatureSensorValueWidget->setRange(-273.1, 100.0, false);
    mUi->lightSensorValueWidget->setRange(0, 40000.0, false);
    mUi->pressureSensorValueWidget->setRange(0, 1100, false);
    mUi->humiditySensorValueWidget->setRange(0, 100, false);
    mUi->proximitySensorValueWidget->setRange(0, 10, false);
    mUi->magNorthWidget->setLocale(QLocale::c());
    mUi->magEastWidget->setLocale(QLocale::c());
    mUi->magVerticalWidget->setLocale(QLocale::c());

    mUi->zRotSlider->setRange(-180.0, 180.0, false);
    mUi->xRotSlider->setRange(-180.0, 180.0, false);
    mUi->yRotSlider->setRange(-180.0, 180.0, false);
    mUi->positionXSlider->setRange(Accelerometer3DWidget::MinX,
                                   Accelerometer3DWidget::MaxX, false);
    mUi->positionYSlider->setRange(Accelerometer3DWidget::MinY,
                                   Accelerometer3DWidget::MaxY, false);
    mUi->positionZSlider->setRange(Accelerometer3DWidget::MinZ,
                                   Accelerometer3DWidget::MaxZ, false);

    connect(mUi->accelWidget, SIGNAL(targetRotationChanged()), this,
            SLOT(propagateAccelWidgetChange()));
    connect(mUi->accelWidget, SIGNAL(targetPositionChanged()), this,
            SLOT(propagateAccelWidgetChange()));

    connect(this, &VirtualSensorsPage::updateResultingValuesRequired,
            this, &VirtualSensorsPage::updateResultingValues);

    connect(this, &VirtualSensorsPage::updateTargetStateRequired,
            this, &VirtualSensorsPage::updateTargetState);

    connect(this, &VirtualSensorsPage::startSensorUpdateTimerRequired,
            this, &VirtualSensorsPage::startSensorUpdateTimer);

    connect(this, &VirtualSensorsPage::stopSensorUpdateTimerRequired,
            this, &VirtualSensorsPage::stopSensorUpdateTimer);

    connect(&mAccelerationTimer, SIGNAL(timeout()),
            this, SLOT(updateSensorValuesInUI()));
    mAccelerationTimer.setInterval(33);
    mAccelerationTimer.stop();

    connect(this, SIGNAL(coarseOrientationChanged(SkinRotation)),
            EmulatorQtWindow::getInstance(),
            SLOT(rotateSkin(SkinRotation)));

    using android::metrics::PeriodicReporter;
    mMetricsReportingToken = PeriodicReporter::get().addCancelableTask(
            60 * 10 * 1000,  // reporting period
            [this](android_studio::AndroidStudioEvent* event) {
                if (mVirtualSensorsUsed) {
                    event->mutable_emulator_details()
                            ->mutable_used_features()
                            ->set_sensors(true);
                    mMetricsReportingToken.reset();  // Report it only once.
                    return true;
                }
                return false;
    });

    if (sSensorsAgent != nullptr) {
        sSensorsAgent->setPhysicalStateAgent(&mQAndroidPhysicalStateAgent);
        mUi->accelWidget->setSensorsAgent(sSensorsAgent);
    }

    updateSensorValuesInUI();

    if (avdInfo_getAvdFlavor(android_avdInfo) == AVD_ANDROID_AUTO) {
        mUi->tabWidget->removeTab(kAccelerometerTabIndex);
    }
}

VirtualSensorsPage::~VirtualSensorsPage() {
    // Unregister for physical state change callbacks.
    if (sSensorsAgent != nullptr) {
        sSensorsAgent->setPhysicalStateAgent(nullptr);
    }
}

void VirtualSensorsPage::showEvent(QShowEvent* event) {
    emit windowVisible();
    mFirstShow = false;
}

void VirtualSensorsPage::on_rotateToPortrait_clicked() {
    setCoarseOrientation(ANDROID_COARSE_PORTRAIT);
}

void VirtualSensorsPage::on_rotateToLandscape_clicked() {
    setCoarseOrientation(ANDROID_COARSE_LANDSCAPE);
}

void VirtualSensorsPage::on_rotateToReversePortrait_clicked() {
    setCoarseOrientation(ANDROID_COARSE_REVERSE_PORTRAIT);
}

void VirtualSensorsPage::on_rotateToReverseLandscape_clicked() {
    setCoarseOrientation(ANDROID_COARSE_REVERSE_LANDSCAPE);
}

void VirtualSensorsPage::setSensorsAgent(const QAndroidSensorsAgent* agent) {
    if (sSensorsAgent != nullptr) {
        sSensorsAgent->setPhysicalStateAgent(nullptr);
    }
    sSensorsAgent = agent;

    // Currently, all our parameter values match the initial values that the
    // system image starts with. If we want to modify any of these initial
    // values, we should do it here, as soon as the agent is available.
}

//
// Helper functions
//

float VirtualSensorsPage::getPhysicalParameterTarget(
        PhysicalParameter parameter_id) {
    return getPhysicalParameterTargetVec3(parameter_id).x;
}

glm::vec3 VirtualSensorsPage::getPhysicalParameterTargetVec3(
        PhysicalParameter parameter_id) {
    glm::vec3 result;
    if (sSensorsAgent) {
        sSensorsAgent->getPhysicalParameter(parameter_id, &result.x, &result.y,
                                            &result.z,
                                            PARAMETER_VALUE_TYPE_TARGET);
    }

    return result;
}

void VirtualSensorsPage::setPhysicalParameterTarget(
        PhysicalParameter parameter_id,
        PhysicalInterpolation mode,
        double v1,
        double v2,
        double v3) {
    reportVirtualSensorsInteraction();
    if (sSensorsAgent) {
        mIsUIModifyingPhysicalState = true;
        sSensorsAgent->setPhysicalParameterTarget(
                parameter_id,
                static_cast<float>(v1),
                static_cast<float>(v2),
                static_cast<float>(v3),
                mode);
        mIsUIModifyingPhysicalState = false;
    }
}

void VirtualSensorsPage::setCoarseOrientation(
        AndroidCoarseOrientation orientation) {
    reportVirtualSensorsInteraction();
    if (sSensorsAgent) {
        sSensorsAgent->setCoarseOrientation(static_cast<int>(orientation));
    }
}

//
// Slots
//

void VirtualSensorsPage::on_temperatureSensorValueWidget_valueChanged(
        double value) {
    setPhysicalParameterTarget(
            PHYSICAL_PARAMETER_TEMPERATURE,
            PHYSICAL_INTERPOLATION_SMOOTH,
            value);
}

void VirtualSensorsPage::on_proximitySensorValueWidget_valueChanged(
        double value) {
    setPhysicalParameterTarget(
            PHYSICAL_PARAMETER_PROXIMITY,
            PHYSICAL_INTERPOLATION_SMOOTH,
            value);
}

void VirtualSensorsPage::on_lightSensorValueWidget_valueChanged(double value) {
    setPhysicalParameterTarget(
            PHYSICAL_PARAMETER_LIGHT,
            PHYSICAL_INTERPOLATION_SMOOTH,
            value);
}

void VirtualSensorsPage::on_pressureSensorValueWidget_valueChanged(
    double value) {
    setPhysicalParameterTarget(
            PHYSICAL_PARAMETER_PRESSURE,
            PHYSICAL_INTERPOLATION_SMOOTH,
            value);
}

void VirtualSensorsPage::on_humiditySensorValueWidget_valueChanged(
    double value) {
    setPhysicalParameterTarget(
            PHYSICAL_PARAMETER_HUMIDITY,
            PHYSICAL_INTERPOLATION_SMOOTH,
            value);
}

void VirtualSensorsPage::on_magNorthWidget_valueChanged(double value) {
    setPhysicalParameterTarget(
            PHYSICAL_PARAMETER_MAGNETIC_FIELD,
            PHYSICAL_INTERPOLATION_SMOOTH,
            mUi->magNorthWidget->value(),
            mUi->magEastWidget->value(),
            mUi->magVerticalWidget->value());
}

void VirtualSensorsPage::on_magEastWidget_valueChanged(double value) {
    setPhysicalParameterTarget(
            PHYSICAL_PARAMETER_MAGNETIC_FIELD,
            PHYSICAL_INTERPOLATION_SMOOTH,
            mUi->magNorthWidget->value(),
            mUi->magEastWidget->value(),
            mUi->magVerticalWidget->value());
}

void VirtualSensorsPage::on_magVerticalWidget_valueChanged(double value) {
    setPhysicalParameterTarget(
            PHYSICAL_PARAMETER_MAGNETIC_FIELD,
            PHYSICAL_INTERPOLATION_SMOOTH,
            mUi->magNorthWidget->value(),
            mUi->magEastWidget->value(),
            mUi->magVerticalWidget->value());
}

void VirtualSensorsPage::on_zRotSlider_valueChanged(double) {
    propagateSlidersChange();
}

void VirtualSensorsPage::on_xRotSlider_valueChanged(double) {
    propagateSlidersChange();
}

void VirtualSensorsPage::on_yRotSlider_valueChanged(double) {
    propagateSlidersChange();
}

void VirtualSensorsPage::on_positionXSlider_valueChanged(double) {
    propagateSlidersChange();
}

void VirtualSensorsPage::on_positionYSlider_valueChanged(double) {
    propagateSlidersChange();
}

void VirtualSensorsPage::on_positionZSlider_valueChanged(double) {
    propagateSlidersChange();
}

void VirtualSensorsPage::updateTargetState() {
    glm::vec3 position =
            getPhysicalParameterTargetVec3(PHYSICAL_PARAMETER_POSITION);
    position *= (1.f / kMetersPerInch);

    glm::vec3 eulerDegrees =
            getPhysicalParameterTargetVec3(PHYSICAL_PARAMETER_ROTATION);

    const glm::quat rotation = fromEulerAnglesXYZ(glm::radians(eulerDegrees));

    mUi->accelWidget->setTargetPosition(position);
    mUi->accelWidget->setTargetRotation(rotation);

    // Convert the rotation to a quaternion so to simplify comparing for
    // equality.
    const glm::quat oldRotation =
            fromEulerAnglesXYZ(glm::radians(mSlidersTargetRotation));

    mSlidersUseCurrent = !vecNearEqual(position, mSlidersTargetPosition) ||
                         !quaternionNearEqual(rotation, oldRotation);

    if (!mIsUIModifyingPhysicalState) {
        updateUIFromModelCurrentState();
    }
}

void VirtualSensorsPage::startSensorUpdateTimer() {
    mBypassOrientationChecks = mVirtualSceneControlsEngaged;
    mAccelerationTimer.start();
}

void VirtualSensorsPage::stopSensorUpdateTimer() {
    mBypassOrientationChecks = false;
    mAccelerationTimer.stop();

    // Do one last sync with the UI, but do it outside of this callback to
    // prevent a deadlock.
    QTimer::singleShot(0, this, SLOT(updateSensorValuesInUI()));
}

void VirtualSensorsPage::onVirtualSceneControlsEngaged(bool engaged) {
    if (engaged) {
        mBypassOrientationChecks = true;
        mVirtualSceneControlsEngaged = true;
    } else {
        mVirtualSceneControlsEngaged = false;
    }
}

void VirtualSensorsPage::reportVirtualSensorsInteraction() {
    if (!mFirstShow) {
        mVirtualSensorsUsed = true;
        if (!mLastInteractionElapsed.isValid() ||
            mLastInteractionElapsed.elapsed() >
                    kMinInteractionTimeMilliseconds) {
            emit virtualSensorsInteraction();
            mLastInteractionElapsed.start();
        }
    }
}

void VirtualSensorsPage::setTargetHeadingDegrees(double heading) {
    while (heading > 180.0) {
        heading -= 360.0;
    }
    while (heading <= -180.0) {
        heading += 360.0;
    }
    setPhysicalParameterTarget(PHYSICAL_PARAMETER_ROTATION,
                               PHYSICAL_INTERPOLATION_STEP,
                               -90.0, 0.0, -heading);
}

void VirtualSensorsPage::onTargetStateChanged() {
    emit updateTargetStateRequired();
}

void VirtualSensorsPage::onPhysicalStateChanging() {
    emit startSensorUpdateTimerRequired();
}

void VirtualSensorsPage::onPhysicalStateStabilized() {
    emit stopSensorUpdateTimerRequired();
}

void VirtualSensorsPage::onTargetStateChanged(void* context) {
    if (context != nullptr) {
        VirtualSensorsPage* virtual_sensors_page =
                reinterpret_cast<VirtualSensorsPage*>(context);
        virtual_sensors_page->onTargetStateChanged();
    }
}

void VirtualSensorsPage::onPhysicalStateChanging(void* context) {
    if (context != nullptr) {
        VirtualSensorsPage* virtual_sensors_page =
                reinterpret_cast<VirtualSensorsPage*>(context);
        virtual_sensors_page->onPhysicalStateChanging();
    }
}

void VirtualSensorsPage::onPhysicalStateStabilized(void* context) {
    if (context != nullptr) {
        VirtualSensorsPage* virtual_sensors_page =
                reinterpret_cast<VirtualSensorsPage*>(context);
        virtual_sensors_page->onPhysicalStateStabilized();
    }
}

void VirtualSensorsPage::updateResultingValues(glm::vec3 acceleration,
                                               glm::vec3 gyroscope,
                                               glm::vec3 device_magnetic_vector) {

    static const QString rotation_labels[] = {
        "ROTATION_0",
        "ROTATION_90",
        "ROTATION_180",
        "ROTATION_270"
    };

    // Update labels with new values.
    QString table_html;
    QTextStream table_html_stream(&table_html);
    table_html_stream.setRealNumberPrecision(2);
    table_html_stream.setNumberFlags(table_html_stream.numberFlags() |
                                     QTextStream::ForcePoint);
    table_html_stream.setRealNumberNotation(QTextStream::FixedNotation);
    table_html_stream
        << "<table border=\"0\""
        << "       cellpadding=\"3\" style=\"font-size:" <<
           Ui::stylesheetFontSize(Ui::FontSize::Medium) << "\">"
        << "<tr>"
        << "<td>" << tr("Accelerometer (m/s<sup>2</sup>)") << ":</td>"
        << "<td align=left>" << acceleration.x << "</td>"
        << "<td align=left>" << acceleration.y << "</td>"
        << "<td align=left>" << acceleration.z << "</td></tr>"
        << "<tr>"
        << "<td>" << tr("Gyroscope (rad/s)") << ":</td>"
        << "<td align=left>" << gyroscope.x << "</td>"
        << "<td align=left>" << gyroscope.y << "</td>"
        << "<td align=left>" << gyroscope.z << "</td></tr>"
        << "<tr>"
        << "<td>" << tr("Magnetometer (&mu;T)") << ":</td>"
        << "<td align=left>" << device_magnetic_vector.x << "</td>"
        << "<td align=left>" << device_magnetic_vector.y << "</td>"
        << "<td align=left>" << device_magnetic_vector.z << "</td></tr>"
        << "<tr><td>" << tr("Rotation")
        << ":</td><td colspan = \"3\" align=left>"
        << rotation_labels[mCoarseOrientation - SKIN_ROTATION_0]
        << "</td></tr>"
        << "</table>";
    mUi->resultingAccelerometerValues->setText(table_html);
}

/*
 * Propagate a UI change from the accel widget to the sliders and model.
 */
void VirtualSensorsPage::propagateAccelWidgetChange() {
    reportVirtualSensorsInteraction();
    updateModelFromAccelWidget(PHYSICAL_INTERPOLATION_SMOOTH);
}

/*
 * Propagate a UI change from the sliders to the accel widget and model.
 */
void VirtualSensorsPage::propagateSlidersChange() {
    reportVirtualSensorsInteraction();
    updateModelFromSliders(PHYSICAL_INTERPOLATION_SMOOTH);
}

/*
 * Send the accel widget's position and rotation to the model as the new
 * targets.
 */
void VirtualSensorsPage::updateModelFromAccelWidget(
        PhysicalInterpolation mode) {
    const glm::vec3& position =
            kMetersPerInch * mUi->accelWidget->targetPosition();
    const glm::vec3 rotationDegrees =
            glm::degrees(toEulerAnglesXYZ(mUi->accelWidget->targetRotation()));

    const glm::vec3 currentPosition =
            getPhysicalParameterTargetVec3(PHYSICAL_PARAMETER_POSITION);
    if (!vecNearEqual(position, currentPosition)) {
        setPhysicalParameterTarget(PHYSICAL_PARAMETER_POSITION, mode,
                                   position.x, position.y, position.z);
    }

    const glm::vec3 currentRotationDegrees =
            getPhysicalParameterTargetVec3(PHYSICAL_PARAMETER_ROTATION);
    if (!vecNearEqual(rotationDegrees, currentRotationDegrees)) {
        setPhysicalParameterTarget(PHYSICAL_PARAMETER_ROTATION, mode,
                                   rotationDegrees.x, rotationDegrees.y,
                                   rotationDegrees.z);
    }
}

/*
 * Send the slider position and rotation to the model as the new targets.
 */
void VirtualSensorsPage::updateModelFromSliders(PhysicalInterpolation mode) {
    glm::vec3 position(mUi->positionXSlider->getValue(),
                       mUi->positionYSlider->getValue(),
                       mUi->positionZSlider->getValue());

    const glm::vec3 rotationDegrees(mUi->xRotSlider->getValue(),
                                    mUi->yRotSlider->getValue(),
                                    mUi->zRotSlider->getValue());

    mSlidersTargetPosition = position;
    mSlidersTargetRotation = rotationDegrees;

    position = kMetersPerInch * position;

    setPhysicalParameterTarget(PHYSICAL_PARAMETER_POSITION, mode,
            position.x, position.y, position.z);
    setPhysicalParameterTarget(PHYSICAL_PARAMETER_ROTATION, mode,
            rotationDegrees.x, rotationDegrees.y, rotationDegrees.z);
}

/*
 * Update the UI to reflect the underlying model state.
 */
void VirtualSensorsPage::updateUIFromModelCurrentState() {
    if (sSensorsAgent != nullptr) {
        glm::vec3 position;
        sSensorsAgent->getPhysicalParameter(
                PHYSICAL_PARAMETER_POSITION, &position.x, &position.y,
                &position.z, PARAMETER_VALUE_TYPE_CURRENT);
        position = (1.f / kMetersPerInch) * position;

        glm::vec3 eulerDegrees;
        sSensorsAgent->getPhysicalParameter(
                PHYSICAL_PARAMETER_ROTATION, &eulerDegrees.x, &eulerDegrees.y,
                &eulerDegrees.z, PARAMETER_VALUE_TYPE_CURRENT);

        mUi->accelWidget->update();

        if (mSlidersUseCurrent) {
            mUi->xRotSlider->setValue(eulerDegrees.x, false);
            mUi->yRotSlider->setValue(eulerDegrees.y, false);
            mUi->zRotSlider->setValue(eulerDegrees.z, false);

            mUi->positionXSlider->setValue(position.x, false);
            mUi->positionYSlider->setValue(position.y, false);
            mUi->positionZSlider->setValue(position.z, false);
        }

        mUi->temperatureSensorValueWidget->setValue(
                getPhysicalParameterTarget(PHYSICAL_PARAMETER_TEMPERATURE),
                false);

        glm::vec3 magneticField = getPhysicalParameterTargetVec3(
                PHYSICAL_PARAMETER_MAGNETIC_FIELD);
        mUi->magNorthWidget->blockSignals(true);
        mUi->magNorthWidget->setValue(magneticField.x);
        mUi->magNorthWidget->blockSignals(false);
        mUi->magEastWidget->blockSignals(true);
        mUi->magEastWidget->setValue(magneticField.y);
        mUi->magEastWidget->blockSignals(false);
        mUi->magVerticalWidget->blockSignals(true);
        mUi->magVerticalWidget->setValue(magneticField.z);
        mUi->magVerticalWidget->blockSignals(false);

        mUi->proximitySensorValueWidget->setValue(
                getPhysicalParameterTarget(PHYSICAL_PARAMETER_PROXIMITY),
                false);
        mUi->lightSensorValueWidget->setValue(
                getPhysicalParameterTarget(PHYSICAL_PARAMETER_LIGHT), false);
        mUi->pressureSensorValueWidget->setValue(
                getPhysicalParameterTarget(PHYSICAL_PARAMETER_PRESSURE), false);
        mUi->humiditySensorValueWidget->setValue(
                getPhysicalParameterTarget(PHYSICAL_PARAMETER_HUMIDITY), false);
    }
}

/*
 * Update the sensor readings in the UI to match the current readings from the
 * inertial model.
 */
void VirtualSensorsPage::updateSensorValuesInUI() {
    updateUIFromModelCurrentState();

    if (sSensorsAgent != nullptr) {
        sSensorsAgent->advanceTime();

        glm::vec3 gravity_vector(0.0f, 9.81f, 0.0f);

        glm::vec3 device_accelerometer;
        sSensorsAgent->getSensor(ANDROID_SENSOR_ACCELERATION,
                &device_accelerometer.x,
                &device_accelerometer.y,
                &device_accelerometer.z);

        if (!mBypassOrientationChecks) {
            glm::vec3 normalized_accelerometer =
                    glm::normalize(device_accelerometer);

            // Update the "rotation" label according to the current acceleraiton.
            static const std::array<std::pair<glm::vec3, SkinRotation>, 4> directions {
                std::make_pair(glm::vec3(0.0f, 1.0f, 0.0f), SKIN_ROTATION_0),
                std::make_pair(glm::vec3(-1.0f, 0.0f, 0.0f), SKIN_ROTATION_90),
                std::make_pair(glm::vec3(0.0f, -1.0f, 0.0f), SKIN_ROTATION_180),
                std::make_pair(glm::vec3(1.0f, 0.0f, 0.0f), SKIN_ROTATION_270)
            };
            QString rotation_label;
            SkinRotation coarse_orientation = mCoarseOrientation;
            for (const auto& v : directions) {
                if (fabs(glm::dot(normalized_accelerometer, v.first) - 1.f) <
                        0.1f) {
                    coarse_orientation = v.second;
                    break;
                }
            }
            if (coarse_orientation != mCoarseOrientation) {
                mCoarseOrientation = coarse_orientation;
                // Signal to the extended-window to rotate the emulator window
                // since an orientation has been detected in the sensor values.
                emit(coarseOrientationChanged(mCoarseOrientation));
            }
        }

        glm::vec3 device_magnetometer;
        sSensorsAgent->getSensor(ANDROID_SENSOR_MAGNETIC_FIELD,
                &device_magnetometer.x,
                &device_magnetometer.y,
                &device_magnetometer.z);

        glm::vec3 device_gyroscope;
        sSensorsAgent->getSensor(ANDROID_SENSOR_GYROSCOPE,
                &device_gyroscope.x,
                &device_gyroscope.y,
                &device_gyroscope.z);

        // Emit a signal to update the UI. We cannot just update
        // the UI here because the current function is sometimes
        // called from a non-Qt thread.
        // We only block signals for this widget if it's running on the Qt
        // thread, so it's Ok to call the connected function directly.
        if (signalsBlocked()) {
            updateResultingValues(
                    device_accelerometer,
                    device_gyroscope,
                    device_magnetometer);
        } else {
            emit updateResultingValuesRequired(
                    device_accelerometer,
                    device_gyroscope,
                    device_magnetometer);
        }
    }
}

void VirtualSensorsPage::on_accelModeRotate_toggled() {
    reportVirtualSensorsInteraction();
    if (mUi->accelModeRotate->isChecked()) {
        mUi->accelWidget->setOperationMode(
            Accelerometer3DWidget::OperationMode::Rotate);
        mUi->accelerometerSliders->setCurrentIndex(0);
    }
}

void VirtualSensorsPage::on_accelModeMove_toggled() {
    reportVirtualSensorsInteraction();
    if (mUi->accelModeMove->isChecked()) {
        mUi->accelWidget->setOperationMode(
            Accelerometer3DWidget::OperationMode::Move);
        mUi->accelerometerSliders->setCurrentIndex(1);
    }
}

void VirtualSensorsPage::on_helpMagneticField_clicked() {
    QDesktopServices::openUrl(QUrl::fromEncoded(
            "https://developer.android.com/reference/android/hardware/Sensor.html#TYPE_MAGNETIC_FIELD"));
}

void VirtualSensorsPage::on_helpLight_clicked() {
    QDesktopServices::openUrl(QUrl::fromEncoded(
            "https://developer.android.com/reference/android/hardware/Sensor.html#TYPE_LIGHT"));
}

void VirtualSensorsPage::on_helpPressure_clicked() {
    QDesktopServices::openUrl(QUrl::fromEncoded(
            "https://developer.android.com/reference/android/hardware/Sensor.html#TYPE_PRESSURE"));
}

void VirtualSensorsPage::on_helpAmbientTemp_clicked() {
  QDesktopServices::openUrl(QUrl::fromEncoded(
            "https://developer.android.com/reference/android/hardware/Sensor.html#TYPE_AMBIENT_TEMPERATURE"));
}

void VirtualSensorsPage::on_helpProximity_clicked() {
  QDesktopServices::openUrl(QUrl::fromEncoded(
            "https://developer.android.com/reference/android/hardware/Sensor.html#TYPE_PROXIMITY"));
}

void VirtualSensorsPage::on_helpHumidity_clicked() {
  QDesktopServices::openUrl(QUrl::fromEncoded(
            "https://developer.android.com/reference/android/hardware/Sensor.html#TYPE_RELATIVE_HUMIDITY"));
}
