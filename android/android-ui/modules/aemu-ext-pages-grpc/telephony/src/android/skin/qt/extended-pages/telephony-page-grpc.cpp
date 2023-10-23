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
#include "android/skin/qt/extended-pages/telephony-page-grpc.h"

#include <limits.h>
#include <QByteArray>
#include <QChar>
#include <QComboBox>
#include <QCoreApplication>
#include <QIcon>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <cassert>
#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "android/avd/info.h"
#include "android/avd/util.h"
#include "android/console.h"
#include "android/emulation/control/globals_agent.h"
#include "android/emulation/control/utils/EmulatorGrcpClient.h"
#include "android/metrics/UiEventTracker.h"
#include "android/skin/qt/error-dialog.h"
#include "android/skin/qt/extended-pages/common.h"
#include "android/skin/qt/raised-material-button.h"
#include "android/telephony/sms.h"
#include "host-common/qt_ui_defs.h"
#include "studio_stats.pb.h"
#include "ui_telephony-page-grpc.h"

#define MAX_SMS_MSG_SIZE 1024  // Arbitrary emulator limitation
#define MAX_SMS_MSG_SIZE_STRING "1024"

using android::emulation::control::incubating::Call;
using android::emulation::control::incubating::PhoneEvent;
using android::emulation::control::incubating::SmsMessage;

class TelephonyEventGrpc : public QEvent {
public:
    TelephonyEventGrpc(QEvent::Type typeId, int nActive)
        : QEvent(typeId), numActiveCalls(nActive) {}

public:
    int numActiveCalls;
};

TelephonyPageGrpc::TelephonyPageGrpc(QWidget* parent)
    : QWidget(parent),
      mUi(new Ui::TelephonyPageGrpc()),
      mModemClient(android::emulation::control::EmulatorGrpcClient::me()),
      mPhoneTracker(new UiEventTracker(
              android_studio::EmulatorUiEvent::BUTTON_PRESS,
              android_studio::EmulatorUiEvent::EXTENDED_TELEPHONY_TAB)) {
    mUi->setupUi(this);
    mUi->tel_numberBox->setValidator(new PhoneNumberValidator());
    mCustomEventType = (QEvent::Type)QEvent::registerEventType();
    mActiveCall.set_state(Call::CALL_STATE_UNSPECIFIED);

    connect(this, SIGNAL(updateCallState()), this, SLOT(onCallChange()));
    connect(this, SIGNAL(errorMessage(QString)), this,
            SLOT(onErrorMessage(QString)));

    mModemClient.receivePhoneEvents(
            [this](const PhoneEvent* event) {
                if (event->has_active()) {
                    eventLauncher(event->active().calls_size());
                }
            },
            [](auto status) { /* ignored */ });

    // Disable sms button and box for Automotive, since it's not supported
    if ((getConsoleAgents()->settings->avdInfo() &&
         (avdInfo_getAvdFlavor(getConsoleAgents()->settings->avdInfo()) ==
          AVD_ANDROID_AUTO))) {
        SettingsTheme theme = getSelectedTheme();
        setButtonEnabled(mUi->sms_sendButton, theme, false);
        mUi->sms_messageBox->setReadOnly(true);
    }
}

TelephonyPageGrpc::~TelephonyPageGrpc() {}

void TelephonyPageGrpc::on_tel_startEndButton_clicked() {
    mPhoneTracker->increment("CALL");
    if (mActiveCall.state() == Call::CALL_STATE_UNSPECIFIED)
    // Start a call
    {
        // Get rid of spurious characters from the phone number
        // (Allow only '+' and '0'..'9')
        // Note: phoneNumberValidator validates the user's input, but
        // allows some human-readable characters like '.' and ')'.
        // Here we remove that meaningless punctuation.
        QString cleanNumber = mUi->tel_numberBox->currentText().remove(
                QRegularExpression("[^+0-9]"));

        mActiveCall.set_number(cleanNumber.toStdString());
        mModemClient.createCallAsync(
                mActiveCall, [this](absl::StatusOr<Call*> status) {
                    if (!status.ok()) {
                        emit(errorMessage(
                                tr(status.status().message().data())));
                        return;
                    }

                    // Success: Update the state and the UI buttons
                    mActiveCall.set_state(Call::CALL_STATE_ACTIVE);
                    emit(updateCallState());
                });
    } else {
        // End a call
        // Update the state and the UI buttons
        mModemClient.deleteCallAsync(mActiveCall, [this](auto status) {
            if (!status.ok()) {
                emit(errorMessage(tr(status.status().message().data())));
                return;
            }

            // Success: Update the state and the UI buttons
            mActiveCall.set_state(Call::CALL_STATE_UNSPECIFIED);
            emit(updateCallState());
        });
    }
}

void TelephonyPageGrpc::onErrorMessage(QString msg) {
    showErrorDialog(msg, tr("Telephony"));
}

void TelephonyPageGrpc::onCallChange() {
    SettingsTheme theme = getSelectedTheme();
    auto state = mActiveCall.state();
    switch (state) {
        case Call::CALL_STATE_ACTIVE: {
            mUi->tel_numberBox->setEnabled(false);

            setButtonEnabled(mUi->tel_holdCallButton, theme, true);
            // Change the icon and text to "END CALL"
            QIcon theIcon = getIconForCurrentTheme("call_end");
            mUi->tel_startEndButton->setIcon(theIcon);
            mUi->tel_startEndButton->setText(tr("END CALL"));

            mUi->tel_holdCallButton->setProperty("themeIconName",
                                                 "phone_in_talk");

            setButtonEnabled(mUi->tel_holdCallButton, theme, true);
            break;
        };
        case Call::CALL_STATE_UNSPECIFIED: {
            mUi->tel_numberBox->setEnabled(true);
            mUi->tel_holdCallButton->setProperty("themeIconName",
                                                 "phone_paused");

            setButtonEnabled(mUi->tel_holdCallButton, theme, false);
            // Change the icon and text to "CALL DEVICE"
            QIcon theIcon = getIconForCurrentTheme("phone_button");
            mUi->tel_startEndButton->setIcon(theIcon);
            mUi->tel_startEndButton->setText(tr("CALL DEVICE"));
            break;
        };
        case Call::CALL_STATE_HELD: {
            mUi->tel_holdCallButton->setProperty("themeIconName",
                                                 "phone_paused");
            mUi->tel_holdCallButton->setProperty("themeIconName_disabled",
                                                 "phone_paused_disabled");

            setButtonEnabled(mUi->tel_holdCallButton, theme, true);
            break;
        };
        /*case Call::CALL_STATE_DIALING:
        case Call::CALL_STATE_ALERTING:
        case Call::CALL_STATE_INCOMING:
        case Call::CALL_STATE_WAITING:*/
        defaut: /* do nothing */
            __attribute__((__unused__));
            break;
    }
}

void TelephonyPageGrpc::on_tel_holdCallButton_clicked() {
    mPhoneTracker->increment("HOLD");
    SettingsTheme theme = getSelectedTheme();
    switch (mActiveCall.state()) {
        case Call::CALL_STATE_ACTIVE:
            // Active --> On hold
            mActiveCall.set_state(Call::CALL_STATE_HELD);
            mModemClient.updateCallAsync(
                    mActiveCall, [this](absl::StatusOr<Call*> status) {
                        if (!status.ok()) {
                            emit(errorMessage(
                                    tr(status.status().message().data())));
                            return;
                        }
                        emit(updateCallState());
                    });
            break;
        case Call::CALL_STATE_HELD:
            // On hold --> Active
            mActiveCall.set_state(Call::CALL_STATE_ACTIVE);
            mModemClient.updateCallAsync(
                    mActiveCall, [this](absl::StatusOr<Call*> status) {
                        if (!status.ok()) {
                            emit(errorMessage(
                                    tr(status.status().message().data())));
                            return;
                        }
                        emit(updateCallState());
                    });
            break;
        default:;
    }
}

QValidator::State TelephonyPageGrpc::PhoneNumberValidator::validate(
        QString& input,
        int& pos) const {
    bool intermediate = false;

    switch (validateAsDigital(input)) {
        case QValidator::Acceptable:
            return QValidator::Acceptable;

        case QValidator::Intermediate:
            intermediate = true;
            break;

        case QValidator::Invalid:
            break;
    }

    switch (validateAsAlphanumeric(input)) {
        case QValidator::Acceptable:
            return QValidator::Acceptable;

        case QValidator::Intermediate:
            intermediate = true;
            break;

        case QValidator::Invalid:
            break;
    }

    if (intermediate) {
        return QValidator::Intermediate;
    }

    return QValidator::Invalid;
}

QValidator::State TelephonyPageGrpc::PhoneNumberValidator::validateAsDigital(
        const QString& input) {
    int numDigits = 0;
    const int MAX_DIGITS = 16;
    static const QString acceptable_non_digits = "-.()/ +";

    if (input.length() >= 32) {
        return QValidator::Invalid;
    }

    for (int i = 0; i < input.length(); i++) {
        const QChar c = input[i];
        if (c.isDigit()) {
            numDigits++;
            if (numDigits > MAX_DIGITS) {
                return QValidator::Invalid;
            }
        } else if (c == '+' && i != 0) {
            // '+' is only allowed as the first character
            return QValidator::Invalid;
        } else if (!acceptable_non_digits.contains(c)) {
            return QValidator::Invalid;
        }
    }

    return ((numDigits > 0) ? QValidator::Acceptable
                            : QValidator::Intermediate);
}

QValidator::State
TelephonyPageGrpc::PhoneNumberValidator::validateAsAlphanumeric(
        const QString& input) {
    // Alphanumeric address is BITS_PER_SMS_CHAR bits per symbol

    if (input.length() == 0) {
        return QValidator::Intermediate;
    }

    if (input.length() >
        (SMS_ADDRESS_MAX_SIZE * CHAR_BIT / BITS_PER_SMS_CHAR)) {
        return QValidator::Invalid;
    }

    for (int i = 0; i < input.length(); i++) {
        if (!is_in_gsm_default_alphabet(input[i].unicode())) {
            return QValidator::Invalid;
        }
    }

    return QValidator::Acceptable;
}

void TelephonyPageGrpc::on_sms_sendButton_clicked() {
    // Get the "from" number
    mPhoneTracker->increment("SMS");

    // Get the text of the message
    const std::string theMessage =
            mUi->sms_messageBox->toPlainText().toStdString();

    // Convert the message text to UTF-8
    const QByteArray utf8 = mUi->sms_messageBox->toPlainText().toUtf8();
    const unsigned char* utf8Message =
            reinterpret_cast<const unsigned char*>(utf8.data());
    int nUtf8Chars = utf8.size();

    if (theMessage.empty()) {
        showErrorDialog(tr("The message is empty.<br>Please enter a message."),
                        tr("SMS"));
        return;
    }

    if (nUtf8Chars < 0) {
        showErrorDialog(tr("The message contains invalid characters."),
                        tr("SMS"));
        return;
    }

    if (nUtf8Chars > MAX_SMS_MSG_SIZE) {
        // Note: 'sms_utf8_from_message_str' did not overwrite our buffer
        showErrorDialog(tr("Android Emulator truncated this message "
                           "to " MAX_SMS_MSG_SIZE_STRING " characters."),
                        tr("SMS"));
        nUtf8Chars = MAX_SMS_MSG_SIZE;
    }

    SmsMessage sms;

    QString cleanNumber = mUi->tel_numberBox->currentText().remove(
            QRegularExpression("[^+0-9]"));

    sms.set_number(cleanNumber.toStdString());
    sms.set_text(theMessage);
    mModemClient.receiveSmsAsync(sms, [this](auto status) {
        if (!status.ok()) {
            emit(errorMessage(tr(status.status().message().data())));
        }
    });
}

void TelephonyPageGrpc::eventLauncher(int numActiveCalls) {
    QEvent* newTelephonyEventGrpc =
            new TelephonyEventGrpc(mCustomEventType, numActiveCalls);
    QCoreApplication::postEvent(this, newTelephonyEventGrpc);
}

void TelephonyPageGrpc::customEvent(QEvent* cEvent) {
    assert(cEvent->type() == mCustomEventType);
    TelephonyEventGrpc* tEvent = (TelephonyEventGrpc*)cEvent;
    if (tEvent->numActiveCalls == 0) {
        if (mActiveCall.state() != Call::CALL_STATE_UNSPECIFIED) {
            // The device has no calls but we're active.
            // They must have hung up on us. Push the
            // END CALL button on our side.
            on_tel_startEndButton_clicked();
        }
        // No 'else': If both we and the device have
        // no calls active, there is nothing to do.
        //  } else {
        //      // The device has a call active. If we are
        //      // also active, there's nothing to do.
        //      //
        //      // If we are not active, the device is probably
        //      // initiating a call. Maybe we should give the
        //      // option to accept or reject that call.
        //      //
        //      // Note that the device can deal with multiple
        //      // calls (e.g., call waiting), but our modem
        //      // interface only tells us how many are active.
        //      // We cannot tell which call was ended.
    }
}
