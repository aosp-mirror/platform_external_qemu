// Copyright 2023 The Android Open Source Project
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

#include <QObject>
#include <QString>
#include <QWidget>
#include <memory>

#include "ui_finger-page-grpc.h"

#include "android/emulation/control/utils/SimpleEmulatorControlClient.h"
#include "android/metrics/UiEventTracker.h"

using android::emulation::control::SimpleEmulatorControlClient;
using android::metrics::UiEventTracker;

class FingerPageGrpc : public QWidget {
    Q_OBJECT

public:
    explicit FingerPageGrpc(QWidget* parent = 0);

private slots:
    void on_finger_touchButton_pressed();
    void on_finger_touchButton_released();

private:
    std::unique_ptr<Ui::FingerPageGrpc> mUi;
    std::shared_ptr<UiEventTracker> mFingerTracker;
    SimpleEmulatorControlClient mEmulatorControl;
};
