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
#include "android/emulator-window.h"
#include "android/hw-sensors.h"
#include "android/metrics/PeriodicReporter.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/physics/Physics.h"
#include "android/skin/ui.h"

#include "android/skin/qt/stylesheet.h"

#include <QDesktopServices>
#include <QTextStream>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <array>
#include <cassert>

VirtualSensorsPage::VirtualSensorsPage(QWidget* parent) :
    QWidget(parent),
    mUi(new Ui::VirtualSensorsPage()),
    mSensorsAgent(nullptr)
{
    mQAndroidPhysicalStateAgent.onTargetStateChanged = onTargetStateChanged;
    mQAndroidPhysicalStateAgent.onPhysicalStateChanging =
            onPhysicalStateChanging;
    mQAndroidPhysicalStateAgent.onPhysicalStateStabilized =
            onPhysicalStateStabilized;
    mQAndroidPhysicalStateAgent.context = reinterpret_cast<void*>(this);

    mUi->setupUi(this);
    // The initial values are set here to match the initial
    // values reported by an AVD.
    mUi->temperatureSensorValueWidget->setRange(-273.1, 100.0);
    mUi->temperatureSensorValueWidget->setValue(0.0);
    mUi->lightSensorValueWidget->setRange(0, 40000.0);
    mUi->lightSensorValueWidget->setValue(0.0);
    mUi->pressureSensorValueWidget->setRange(0, 1100);
    mUi->pressureSensorValueWidget->setValue(0.0);
    mUi->humiditySensorValueWidget->setRange(0, 100);
    mUi->humiditySensorValueWidget->setValue(0);
    mUi->proximitySensorValueWidget->setRange(0, 10);
    mUi->proximitySensorValueWidget->setValue(1);
    mUi->magNorthWidget->setLocale(QLocale::c());
    mUi->magEastWidget->setLocale(QLocale::c());
    mUi->magVerticalWidget->setLocale(QLocale::c());

    connect(mUi->accelWidget, SIGNAL(rotationChanged()),
            this, SLOT(propagateAccelWidgetChange()));

    connect(mUi->accelWidget, SIGNAL(positionChanged()),
            this, SLOT(propagateAccelWidgetChange()));

    connect(mUi->positionXSlider, SIGNAL(sliderPressed()),
            this, SLOT(onDragStarted()));
    connect(mUi->positionXSlider, SIGNAL(sliderReleased()),
            this, SLOT(onDragStopped()));

    connect(mUi->positionYSlider, SIGNAL(sliderPressed()),
            this, SLOT(onDragStarted()));
    connect(mUi->positionYSlider, SIGNAL(sliderReleased()),
            this, SLOT(onDragStopped()));

    connect(mUi->zRotSlider, SIGNAL(sliderPressed()),
            this, SLOT(onDragStarted()));
    connect(mUi->zRotSlider, SIGNAL(sliderReleased()),
            this, SLOT(onDragStopped()));

    connect(mUi->xRotSlider, SIGNAL(sliderPressed()),
            this, SLOT(onDragStarted()));
    connect(mUi->xRotSlider, SIGNAL(sliderReleased()),
            this, SLOT(onDragStopped()));

    connect(mUi->yRotSlider, SIGNAL(sliderPressed()),
            this, SLOT(onDragStarted()));
    connect(mUi->yRotSlider, SIGNAL(sliderReleased()),
            this, SLOT(onDragStopped()));

    connect(this, &VirtualSensorsPage::updateResultingValuesRequired,
            this, &VirtualSensorsPage::updateResultingValues);

    connect(this, &VirtualSensorsPage::startSensorUpdateTimerRequired,
            this, &VirtualSensorsPage::startSensorUpdateTimer);

    connect(this, &VirtualSensorsPage::stopSensorUpdateTimerRequired,
            this, &VirtualSensorsPage::stopSensorUpdateTimer);

    connect(&mAccelerationTimer, SIGNAL(timeout()),
            this, SLOT(updateSensorValuesInUI()));
    mAccelerationTimer.setInterval(100);
    mAccelerationTimer.stop();

    mUi->zRotSlider->setRange(-180.0, 180.0);
    mUi->xRotSlider->setRange(-180.0, 180.0);
    mUi->yRotSlider->setRange(-180.0, 180.0);
    mUi->positionXSlider->setRange(Accelerometer3DWidget::MinX,
                                   Accelerometer3DWidget::MaxX);
    mUi->positionYSlider->setRange(Accelerometer3DWidget::MinY,
                                   Accelerometer3DWidget::MaxY);
    // Historically, the AVD starts up with the screen mostly
    // vertical, but tilted back 4.75 degrees. Retain that
    // initial orientation.
    // We need to do this after we call setRange since setRange will trigger
    // on_*Slider_valueChanged which just trigger setRotation from default
    // value of Sliders.
    static const glm::quat initialQuat(glm::vec3(glm::radians(-4.75f), 0.00f, 0.00f));
    mUi->accelWidget->setRotation(glm::mat4_cast(initialQuat));
    updateSlidersFromAccelWidget();
    updateModelFromAccelWidget(PHYSICAL_INTERPOLATION_STEP);

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
}

VirtualSensorsPage::~VirtualSensorsPage() {
    // Unregister for physical state change callbacks.
    if (mSensorsAgent != nullptr) {
        mSensorsAgent->setPhysicalStateAgent(nullptr);
    }
}

void VirtualSensorsPage::showEvent(QShowEvent*) {
    auto layout = skin_ui_get_current_layout(emulator_window_get()->ui);
    if (layout) {
        resetDeviceRotationFromSkinLayout(layout->orientation);
    }

    mFirstShow = false;
}

void VirtualSensorsPage::onSkinLayoutChange(SkinRotation rot) {
    resetDeviceRotationFromSkinLayout(rot);
}

void VirtualSensorsPage::resetDeviceRotationFromSkinLayout(
        SkinRotation orientation) {

    if (mCoarseOrientation == orientation) {
        return;
    }

    float rot = 0.0f;

    // NOTE: the "incorrect" angle values
    // stem from the fact that glm::quat and SKIN_ROTATION_*
    // disagree on which direction is "positive" (skin uses
    // a different coordinate system with origin at top left
    // and X and Y axis pointing right and down respectively).
    switch (orientation) {
    case SKIN_ROTATION_0:
        rot = 0.0f;
        break;
    case SKIN_ROTATION_90:
        rot = -90.0f;
        break;
    case SKIN_ROTATION_180:
        rot = 180.0f;
        break;
    case SKIN_ROTATION_270:
        rot = 90.0f;
        break;
    default:
        assert(0);
    }
    // Rotation reset handler notifies the subscribers, and one of the
    // subscribers is the main window which handles that as a regular
    // rotation command. As we're already handling a rotation command, that
    // would make us to handle it again for the second time, which is
    // very CPU-intensive.
    QSignalBlocker blockSignals(this);
    resetDeviceRotation(
            glm::angleAxis(glm::radians(rot), glm::vec3(0.0f, 0.0f, 1.0f)));
}

void VirtualSensorsPage::resetDeviceRotation(const glm::quat& rotation) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    mUi->accelWidget->setPosition(glm::vec2(0.0f, 0.0f));
    mUi->accelWidget->setRotation(glm::mat4_cast(rotation));
    mUi->accelWidget->update();

    // Note: here we do an instantaneous model update.
    updateSlidersFromAccelWidget();
    updateModelFromAccelWidget(PHYSICAL_INTERPOLATION_STEP);
}

void VirtualSensorsPage::on_rotateToPortrait_clicked() {
    resetDeviceRotation(glm::quat());
}

void VirtualSensorsPage::on_rotateToLandscape_clicked() {
    resetDeviceRotation(
        glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
}

void VirtualSensorsPage::on_rotateToReversePortrait_clicked() {
    resetDeviceRotation(
        glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
}

void VirtualSensorsPage::on_rotateToReverseLandscape_clicked() {
    resetDeviceRotation(
        glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
}

void VirtualSensorsPage::setSensorsAgent(const QAndroidSensorsAgent* agent) {

    if (mSensorsAgent != nullptr) {
        mSensorsAgent->setPhysicalStateAgent(nullptr);
    }
    mSensorsAgent = agent;

    auto layout = skin_ui_get_current_layout(emulator_window_get()->ui);
    if (layout) {
        resetDeviceRotationFromSkinLayout(layout->orientation);
    }

    mSensorsAgent->setPhysicalStateAgent(&mQAndroidPhysicalStateAgent);
}

// Helper function
void VirtualSensorsPage::setPhysicalParameterTarget(
        PhysicalParameter parameter_id,
        PhysicalInterpolation mode,
        double v1,
        double v2,
        double v3) {
    if (mSensorsAgent && !mIsUpdatingUIFromModel) {
        mIsUIModifyingPhysicalState = true;
        mSensorsAgent->setPhysicalParameterTarget(
                parameter_id,
                static_cast<float>(v1),
                static_cast<float>(v2),
                static_cast<float>(v3),
                mode);
        mIsUIModifyingPhysicalState = false;
    }
}

void VirtualSensorsPage::on_temperatureSensorValueWidget_valueChanged(
        double value) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    setPhysicalParameterTarget(
            PHYSICAL_PARAMETER_TEMPERATURE,
            PHYSICAL_INTERPOLATION_SMOOTH,
            value);
}

void VirtualSensorsPage::on_proximitySensorValueWidget_valueChanged(
        double value) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    setPhysicalParameterTarget(
            PHYSICAL_PARAMETER_PROXIMITY,
            PHYSICAL_INTERPOLATION_SMOOTH,
            value);
}

void VirtualSensorsPage::on_lightSensorValueWidget_valueChanged(double value) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    setPhysicalParameterTarget(
            PHYSICAL_PARAMETER_LIGHT,
            PHYSICAL_INTERPOLATION_SMOOTH,
            value);
}

void VirtualSensorsPage::on_pressureSensorValueWidget_valueChanged(
    double value) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    setPhysicalParameterTarget(
            PHYSICAL_PARAMETER_PRESSURE,
            PHYSICAL_INTERPOLATION_SMOOTH,
            value);
}

void VirtualSensorsPage::on_humiditySensorValueWidget_valueChanged(
    double value) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
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

void VirtualSensorsPage::onTargetStateChanged() {
    if (!mIsUIModifyingPhysicalState) {
        mIsUpdatingUIFromModel = true;
        updateAccelWidgetAndSlidersFromModel();
        mIsUpdatingUIFromModel = false;
    }
}

void VirtualSensorsPage::startSensorUpdateTimer() {
    mAccelerationTimer.start();
}

void VirtualSensorsPage::stopSensorUpdateTimer() {
    mAccelerationTimer.stop();
    updateSensorValuesInUI();
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
    updateSlidersFromAccelWidget();
    updateModelFromAccelWidget(PHYSICAL_INTERPOLATION_SMOOTH);
}

/*
 * Propagate a UI change from the sliders to the accel widget and model.
 */
void VirtualSensorsPage::propagateSlidersChange() {
    updateAccelWidgetFromSliders();
    updateModelFromAccelWidget( PHYSICAL_INTERPOLATION_SMOOTH );
}

/*
 * Send the sliders' position and rotation to the accel widget.
 */
void VirtualSensorsPage::updateAccelWidgetFromSliders() {
    glm::vec3 position(mUi->positionXSlider->getValue(),
                       mUi->positionYSlider->getValue(),
                       0.f);
    mUi->accelWidget->setPosition(glm::vec2(position));

    mUi->accelWidget->setRotation(glm::eulerAngleXYZ(
            glm::radians(mUi->xRotSlider->getValue()),
            glm::radians(mUi->yRotSlider->getValue()),
            glm::radians(mUi->zRotSlider->getValue())));

    mUi->accelWidget->update();
}

/*
 * Send the accel widget's position and rotation to the sliders.
 */
void VirtualSensorsPage::updateSlidersFromAccelWidget() {
    const glm::vec2& pos = mUi->accelWidget->position();
    glm::vec3 position(pos, 0.0f);

    glm::vec3 rotationRadians;
    glm::extractEulerAngleXYZ(mUi->accelWidget->rotation(),
            rotationRadians.x, rotationRadians.y, rotationRadians.z);
    const glm::vec3 rotationDegrees = glm::degrees(rotationRadians);

    mUi->xRotSlider->setValue(rotationDegrees.x, false);
    mUi->yRotSlider->setValue(rotationDegrees.y, false);
    mUi->zRotSlider->setValue(rotationDegrees.z, false);

    mUi->positionXSlider->setValue(pos.x, false);
    mUi->positionYSlider->setValue(pos.y, false);
}

constexpr float kMetersPerInch = 0.0254f;

/*
 * Send the accel widget's position and rotation to the model as the new
 * targets.
 */
void VirtualSensorsPage::updateModelFromAccelWidget(PhysicalInterpolation mode) {
    const glm::vec2& position = kMetersPerInch * mUi->accelWidget->position();
    glm::vec3 rotationRadians;
    glm::extractEulerAngleXYZ(mUi->accelWidget->rotation(),
            rotationRadians.x, rotationRadians.y, rotationRadians.z);
    const glm::vec3 rotationDegrees = glm::degrees(rotationRadians);

    setPhysicalParameterTarget(PHYSICAL_PARAMETER_POSITION, mode,
            position.x, position.y, 0.f);
    setPhysicalParameterTarget(PHYSICAL_PARAMETER_ROTATION, mode,
            rotationDegrees.x, rotationDegrees.y, rotationDegrees.z);
}

/*
 * Update the accel widge and sliders to reflect the underlying model state.
 */
void VirtualSensorsPage::updateAccelWidgetAndSlidersFromModel() {
    if (mSensorsAgent != nullptr) {
        glm::vec3 position;
        mSensorsAgent->getPhysicalParameterTarget(PHYSICAL_PARAMETER_POSITION,
                &position.x, &position.y, &position.z);
        position = (1.f / kMetersPerInch) * position;

        glm::vec3 eulerDegrees;
        mSensorsAgent->getPhysicalParameterTarget(PHYSICAL_PARAMETER_ROTATION,
                &eulerDegrees.x, &eulerDegrees.y, &eulerDegrees.z);

        mUi->accelWidget->setPosition(position);
        mUi->accelWidget->setRotation(glm::eulerAngleXYZ(
                glm::radians(eulerDegrees.x),
                glm::radians(eulerDegrees.y),
                glm::radians(eulerDegrees.z)));
        mUi->accelWidget->update();

        mUi->xRotSlider->setValue(eulerDegrees.x, false);
        mUi->yRotSlider->setValue(eulerDegrees.y, false);
        mUi->zRotSlider->setValue(eulerDegrees.z, false);

        mUi->positionXSlider->setValue(position.x, false);
        mUi->positionYSlider->setValue(position.y, false);

        float scratch0, scratch1;

        float temperature;
        mSensorsAgent->getPhysicalParameterTarget(
                PHYSICAL_PARAMETER_TEMPERATURE, &temperature,
                &scratch0, &scratch1);
        mUi->temperatureSensorValueWidget->setValue(temperature);

        glm::vec3 magneticField;
        mSensorsAgent->getPhysicalParameterTarget(
                PHYSICAL_PARAMETER_MAGNETIC_FIELD,
                &magneticField.x, &magneticField.y, &magneticField.z);
        mUi->magNorthWidget->setValue(magneticField.x);
        mUi->magEastWidget->setValue(magneticField.y);
        mUi->magVerticalWidget->setValue(magneticField.z);

        float proximity;
        mSensorsAgent->getPhysicalParameterTarget(
                PHYSICAL_PARAMETER_PROXIMITY, &proximity,
                &scratch0, &scratch1);
        mUi->proximitySensorValueWidget->setValue(proximity);

        float light;
        mSensorsAgent->getPhysicalParameterTarget(
                PHYSICAL_PARAMETER_LIGHT, &light,
                &scratch0, &scratch1);
        mUi->lightSensorValueWidget->setValue(light);

        float pressure;
        mSensorsAgent->getPhysicalParameterTarget(
                PHYSICAL_PARAMETER_PRESSURE, &pressure,
                &scratch0, &scratch1);
        mUi->pressureSensorValueWidget->setValue(pressure);

        float humidity;
        mSensorsAgent->getPhysicalParameterTarget(
                PHYSICAL_PARAMETER_HUMIDITY, &humidity,
                &scratch0, &scratch1);
        mUi->humiditySensorValueWidget->setValue(humidity);
    }
}

/*
 * Update the sensor readings in the UI to match the current readings from the
 * inertial model.
 */
void VirtualSensorsPage::updateSensorValuesInUI() {
    if (mSensorsAgent != nullptr) {
        glm::vec3 gravity_vector(0.0f, 9.81f, 0.0f);

        glm::vec3 device_accelerometer;
        mSensorsAgent->getSensor(ANDROID_SENSOR_ACCELERATION,
                &device_accelerometer.x,
                &device_accelerometer.y,
                &device_accelerometer.z);
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
            emit(coarseOrientationChanged(mCoarseOrientation));
        }

        glm::vec3 device_magnetometer;
        mSensorsAgent->getSensor(ANDROID_SENSOR_MAGNETIC_FIELD,
                &device_magnetometer.x,
                &device_magnetometer.y,
                &device_magnetometer.z);

        glm::vec3 device_gyroscope;
        mSensorsAgent->getSensor(ANDROID_SENSOR_GYROSCOPE,
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

void VirtualSensorsPage::onDragStarted() {
    mIsDragging = true;
}

void VirtualSensorsPage::onDragStopped() {
    mIsDragging = false;
    propagateSlidersChange();
}

void VirtualSensorsPage::on_accelModeRotate_toggled() {
    if (mUi->accelModeRotate->isChecked()) {
        mUi->accelWidget->setOperationMode(
            Accelerometer3DWidget::OperationMode::Rotate);
        mUi->accelerometerSliders->setCurrentIndex(0);
    }
}

void VirtualSensorsPage::on_accelModeMove_toggled() {
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
