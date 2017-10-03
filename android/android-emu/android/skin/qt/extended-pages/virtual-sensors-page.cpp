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
#include "android/skin/ui.h"

#include "android/skin/qt/stylesheet.h"

#include <QDesktopServices>
#include <QTextStream>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <cassert>

VirtualSensorsPage::VirtualSensorsPage(QWidget* parent) :
    QWidget(parent),
    mUi(new Ui::VirtualSensorsPage()),
    mSensorsAgent(nullptr)
{
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

    syncUIAndUpdateModel();

    connect(mUi->accelWidget, SIGNAL(rotationChanged()),
            this, SLOT(syncUIAndUpdateModel()));
    connect(mUi->accelWidget, SIGNAL(positionChanged()),
            this, SLOT(syncUIAndUpdateModel()));

    connect(mUi->accelWidget, SIGNAL(dragStopped()),
            this, SLOT(onDragStopped()));
    connect(mUi->accelWidget, SIGNAL(dragStarted()),
            this, SLOT(onDragStarted()));

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

    connect(&mAccelerationTimer, SIGNAL(timeout()),
            this, SLOT(updateAccelerations()));
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
    mUi->accelWidget->setRotation(initialQuat);
    syncUIAndUpdateModel();

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

void VirtualSensorsPage::showEvent(QShowEvent*) {
    auto layout = skin_ui_get_current_layout(emulator_window_get()->ui);
    if (layout) {
        resetAccelerometerRotationFromSkinLayout(layout->orientation);
    }
    mFirstShow = false;
}

void VirtualSensorsPage::setLayoutChangeNotifier(
        QObject* layout_change_notifier) {
    connect(layout_change_notifier, SIGNAL(layoutChanged(SkinRotation)),
            this, SLOT(onSkinLayoutChange(SkinRotation)));
}

void VirtualSensorsPage::onSkinLayoutChange(SkinRotation rot) {
    resetAccelerometerRotationFromSkinLayout(rot);
}

void VirtualSensorsPage::resetAccelerometerRotationFromSkinLayout(
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
    resetAccelerometerRotation(
            glm::angleAxis(glm::radians(rot), glm::vec3(0.0f, 0.0f, 1.0f)));
}

void VirtualSensorsPage::resetAccelerometerRotation(const glm::quat& rotation) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    mUi->accelWidget->setPosition(glm::vec2(0.0f, 0.0f));
    mUi->accelWidget->setRotation(rotation);
    mUi->accelWidget->update();
    syncUIAndUpdateModel();
}

void VirtualSensorsPage::on_rotateToPortrait_clicked() {
    resetAccelerometerRotation(glm::quat());
}

void VirtualSensorsPage::on_rotateToLandscape_clicked() {
    resetAccelerometerRotation(
        glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
}

void VirtualSensorsPage::on_rotateToReversePortrait_clicked() {
    resetAccelerometerRotation(
        glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
}

void VirtualSensorsPage::on_rotateToReverseLandscape_clicked() {
    resetAccelerometerRotation(
        glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
}

void VirtualSensorsPage::setSensorsAgent(const QAndroidSensorsAgent* agent) {
    mSensorsAgent = agent;

    mInertialModel.setSensorsAgent(agent);
}

// Helper function
static void setSensorValue(
        const QAndroidSensorsAgent* agent,
        AndroidSensor sensor_id,
        double v1,
        double v2 = 0.0,
        double v3 = 0.0) {
  if (agent) {
        agent->setSensor(sensor_id,
                         static_cast<float>(v1),
                         static_cast<float>(v2),
                         static_cast<float>(v3));
    }
}

void VirtualSensorsPage::on_temperatureSensorValueWidget_valueChanged(
        double value) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    setSensorValue(mSensorsAgent, ANDROID_SENSOR_TEMPERATURE, value);
}

void VirtualSensorsPage::on_proximitySensorValueWidget_valueChanged(
        double value) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    setSensorValue(mSensorsAgent, ANDROID_SENSOR_PROXIMITY, value);
}

void VirtualSensorsPage::on_lightSensorValueWidget_valueChanged(double value) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    setSensorValue(mSensorsAgent, ANDROID_SENSOR_LIGHT, value);
}

void VirtualSensorsPage::on_pressureSensorValueWidget_valueChanged(
    double value) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    setSensorValue(mSensorsAgent, ANDROID_SENSOR_PRESSURE, value);
}

void VirtualSensorsPage::on_humiditySensorValueWidget_valueChanged(
    double value) {
    if (!mFirstShow) mVirtualSensorsUsed = true;
    setSensorValue(mSensorsAgent, ANDROID_SENSOR_HUMIDITY, value);
}

void VirtualSensorsPage::on_magNorthWidget_valueChanged(double value) {
    syncUIAndUpdateModel();
}

void VirtualSensorsPage::on_magEastWidget_valueChanged(double value) {
    syncUIAndUpdateModel();
}

void VirtualSensorsPage::on_magVerticalWidget_valueChanged(double value) {
    syncUIAndUpdateModel();
}

void VirtualSensorsPage::setAccelerometerRotationFromSliders() {
    const glm::quat rotation(glm::vec3(
            glm::radians(mUi->xRotSlider->getValue()),
            glm::radians(mUi->yRotSlider->getValue()),
            glm::radians(mUi->zRotSlider->getValue())));
    mInertialModel.setTargetRotation(rotation);
    mUi->accelWidget->setRotation(rotation);
    mUi->accelWidget->update();
}

void VirtualSensorsPage::on_zRotSlider_valueChanged(double) {
    setAccelerometerRotationFromSliders();
}

void VirtualSensorsPage::on_xRotSlider_valueChanged(double) {
    setAccelerometerRotationFromSliders();
}

void VirtualSensorsPage::on_yRotSlider_valueChanged(double) {
    setAccelerometerRotationFromSliders();
}

void VirtualSensorsPage::setPhonePositionFromSliders() {
    glm::vec3 position(mUi->positionXSlider->getValue(),
                       mUi->positionYSlider->getValue(),
                       0.f);
    mInertialModel.setTargetPosition(position);
    mUi->accelWidget->setPosition(glm::vec2(position));
    mUi->accelWidget->update();
}

void VirtualSensorsPage::on_positionXSlider_valueChanged(double) {
    setPhonePositionFromSliders();
}

void VirtualSensorsPage::on_positionYSlider_valueChanged(double) {
    setPhonePositionFromSliders();
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

void VirtualSensorsPage::syncUIAndUpdateModel() {
    const glm::vec2& pos = mUi->accelWidget->position();
    glm::vec3 position(pos, 0.0f);
    const glm::quat& rotation = mUi->accelWidget->rotation();

    mInertialModel.setTargetPosition(position);
    mInertialModel.setTargetRotation(rotation);
    mInertialModel.setMagneticValue(
            mUi->magEastWidget->value(),
            mUi->magVerticalWidget->value(),
            -mUi->magNorthWidget->value());

    glm::vec3 eulerAngles(glm::eulerAngles(rotation));
    mUi->xRotSlider->setValue(glm::degrees(eulerAngles.x), false);
    mUi->yRotSlider->setValue(glm::degrees(eulerAngles.y), false);
    mUi->zRotSlider->setValue(glm::degrees(eulerAngles.z), false);

    mUi->positionXSlider->setValue(pos.x, false);
    mUi->positionYSlider->setValue(pos.y, false);

    glm::vec3 gravity_vector(0.0f, 9.81f, 0.0f);
    glm::quat device_rotation_quat = mInertialModel.getRotation();
    glm::vec3 device_gravity_vector =
        glm::conjugate(device_rotation_quat) * gravity_vector;

    // Update the "rotation" label according to the simulated gravity vector.
    glm::vec3 normalized_gravity = glm::normalize(device_gravity_vector);
    static const std::array<std::pair<glm::vec3, SkinRotation>, 4> directions {
      std::make_pair(glm::vec3(0.0f, 1.0f, 0.0f), SKIN_ROTATION_0),
      std::make_pair(glm::vec3(-1.0f, 0.0f, 0.0f), SKIN_ROTATION_90),
      std::make_pair(glm::vec3(0.0f, -1.0f, 0.0f), SKIN_ROTATION_180),
      std::make_pair(glm::vec3(1.0f, 0.0f, 0.0f), SKIN_ROTATION_270)
    };
    QString rotation_label;
    SkinRotation coarse_orientation = mCoarseOrientation;
    for (const auto& v : directions) {
      if (fabs(glm::dot(normalized_gravity, v.first) - 1.0f) < 0.1f) {
        coarse_orientation = v.second;
        break;
      }
    }
    if (coarse_orientation != mCoarseOrientation) {
      mCoarseOrientation = coarse_orientation;
      emit(coarseOrientationChanged(mCoarseOrientation));
    }

    const glm::vec3& acceleration = mInertialModel.getAcceleration();
    const glm::vec3& device_magnetic_vector =
            mInertialModel.getMagneticVector();
    const glm::vec3& gyroscope =
            mInertialModel.getGyroscope();

    // Emit a signal to update the UI. We cannot just update
    // the UI here because the current function is sometimes
    // called from a non-Qt thread.
    // We only block signals for this widget if it's running on the Qt thread,
    // so it's Ok to call the connected function directly.
    if (signalsBlocked()) {
        updateResultingValues(acceleration, gyroscope, device_magnetic_vector);
    } else {
        emit updateResultingValuesRequired(
                acceleration, gyroscope, device_magnetic_vector);
    }
}

void VirtualSensorsPage::onDragStarted() {
    mAccelerationTimer.start();
}

void VirtualSensorsPage::onDragStopped() {
    // call position changed one last time to zero accelerations.
    syncUIAndUpdateModel();
    mAccelerationTimer.stop();
}

void VirtualSensorsPage::updateAccelerations() {
    mUi->accelWidget->setRotation(
        glm::quat(glm::vec3(
            glm::radians(mUi->xRotSlider->getValue()),
            glm::radians(mUi->yRotSlider->getValue()),
            glm::radians(mUi->zRotSlider->getValue()))));
    syncUIAndUpdateModel();
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
