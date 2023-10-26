// Copyright (C) 2023 The Android Open Source Project
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

#include <QByteArray>
#include <QEvent>         // for QEvent
#include <QString>        // for QString
#include <QValidator>     // for QValidator
#include <QWidget>        // for QWidget
#include <memory>         // for unique_ptr

#include "android/emulation/control/utils/ModemClient.h"
#include "modem_service.pb.h"

class QObject;

namespace android {
namespace metrics {
class UiEventTracker;
}  // namespace metrics
}  // namespace android

namespace Ui {
class TelephonyPageGrpc;
}  // namespace Ui

using android::emulation::control::ModemClient;
using android::emulation::control::incubating::Call;
using android::metrics::UiEventTracker;
class TelephonyPageGrpc : public QWidget {
    Q_OBJECT

public:
    explicit TelephonyPageGrpc(QWidget* parent = 0);
    ~TelephonyPageGrpc();

    void eventLauncher(int);

signals:
    void updateCallState();
    void errorMessage(QString msg);

private slots:
    void on_tel_startEndButton_clicked();
    void on_tel_holdCallButton_clicked();
    void on_sms_sendButton_clicked();

    void onCallChange();
    void onErrorMessage(QString msg);

    void customEvent(QEvent*);

private:
    class PhoneNumberValidator : public QValidator {
    public:
        // Validate the input of a telephone number.
        // We allow '+' only in the first position.
        // One or more digits (0-9) are required.
        // Some additional characters are allowed, but ignored.
        State validate(QString& input, int& pos) const;

    private:
        static State validateAsDigital(const QString& input);
        static State validateAsAlphanumeric(const QString& input);
    };

    enum class CallActivity { Inactive, Active, Held };

    std::unique_ptr<Ui::TelephonyPageGrpc> mUi;
    std::shared_ptr<UiEventTracker> mPhoneTracker;
    Call mActiveCall;
    ModemClient mModemClient;
    QEvent::Type mCustomEventType;
};
