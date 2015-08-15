/* Copyright (C) 2015 The Android Open Source Project
 **
 ** This software is licensed under the terms of the GNU General Public
 ** License version 2, as published by the Free Software Foundation, and
 ** may be copied, distributed, and modified under those terms.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 */

// TODO: Allow special characters in the phone number
// TODO: Allow a second initiated call to test call waiting on the device.
// TODO: Spawn a task to listen for call actions from the device: outgoing
//       call, answer, hang up, reject, ...

#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include "android/skin/qt/emulator-qt-window.h"
#include "extended-window.h"
#include "ui_extended.h"

#include <QtWidgets>
#include <QValidator>

void ExtendedWindow::initTelephony()
{
    // Set a function to validate the phone-number input
    extendedUi->tel_numberBox->setValidator(new phoneNumberValidator);
}

void ExtendedWindow::on_tel_startCallButton_clicked()
{
    if (telephonyAgent && telephonyAgent->telephonyCmd) {
        // Command the emulator
        TelephonyResponse tResp;

        // Get rid of spurious characters from the phone number
        // (Allow only '+' and '0'..'9')
        QString cleanNumber = extendedUi->tel_numberBox->currentText().remove(QRegularExpression("[^+0-9]"));

        tResp = telephonyAgent->telephonyCmd(Tel_Op_Init_Call,
                                             cleanNumber.toStdString().c_str());
        if (tResp != Tel_Resp_OK) {
            QErrorMessage *eM = new QErrorMessage;
            eM->showMessage(tr("The call failed"));
            return;
        }
    }

    // Success: Update the state and the UI buttons
    telephonyState.activity = Call_Active;
    telephonyState.phoneNumber = extendedUi->tel_numberBox->currentText();

    extendedUi->tel_numberBox->setEnabled(false);

    setButtonEnabled(extendedUi->tel_startCallButton, false);
    setButtonEnabled(extendedUi->tel_endCallButton,   true);
    setButtonEnabled(extendedUi->tel_holdCallButton,  true);
}

void ExtendedWindow::on_tel_endCallButton_clicked()
{
    // Update the state and the UI buttons
    telephonyState.activity = Call_Inactive;

    extendedUi->tel_numberBox->setEnabled(true);

    extendedUi->tel_holdCallButton->setProperty("themeIconName", "phone_paused");

    setButtonEnabled(extendedUi->tel_startCallButton, true);
    setButtonEnabled(extendedUi->tel_endCallButton,   false);
    setButtonEnabled(extendedUi->tel_holdCallButton,  false);

    if (telephonyAgent && telephonyAgent->telephonyCmd) {
        // Command the emulator
        QString cleanNumber = extendedUi->tel_numberBox->currentText().remove(QRegularExpression("[^+0-9]"));
        TelephonyResponse tResp;
        tResp = telephonyAgent->telephonyCmd(Tel_Op_Disconnect_Call,
                                             cleanNumber.toStdString().c_str());
        if (tResp != Tel_Resp_OK) {
            QErrorMessage *eM = new QErrorMessage;
            eM->showMessage(tr("The end-call failed"));
            return;
        }
    }
}

void ExtendedWindow::on_tel_holdCallButton_clicked()
{
    switch (telephonyState.activity) {
        case Call_Active:
            // Active --> On hold
            telephonyState.activity = Call_Held;

            extendedUi->tel_holdCallButton->setProperty("themeIconName", "phone_in_talk");

            setButtonEnabled(extendedUi->tel_startCallButton, false);
            setButtonEnabled(extendedUi->tel_endCallButton,   true);
            setButtonEnabled(extendedUi->tel_holdCallButton,  true);
            break;
        case Call_Held:
            // On hold --> Active
            telephonyState.activity = Call_Active;

            extendedUi->tel_holdCallButton->setProperty("themeIconName",          "phone_paused");
            extendedUi->tel_holdCallButton->setProperty("themeIconName_disabled", "phone_paused_disabled");

            setButtonEnabled(extendedUi->tel_startCallButton, false);
            setButtonEnabled(extendedUi->tel_endCallButton,   true);
            setButtonEnabled(extendedUi->tel_holdCallButton,  true);
            break;
        default:
            ;
    }
    if (telephonyAgent && telephonyAgent->telephonyCmd) {
        // Command the emulator
        QString cleanNumber = extendedUi->tel_numberBox->currentText().remove(QRegularExpression("[^+0-9]"));
        TelephonyResponse  tResp;
        TelephonyOperation tOp = (telephonyState.activity == Call_Held) ?
                                     Tel_Op_Place_Call_On_Hold : Tel_Op_Take_Call_Off_Hold;

        tResp = telephonyAgent->telephonyCmd(tOp,
                                             cleanNumber.toStdString().c_str());
        if (tResp != Tel_Resp_OK) {
            QErrorMessage *eM = new QErrorMessage;
            eM->showMessage(tr("The call hold failed"));
            return;
        }
    }
}
