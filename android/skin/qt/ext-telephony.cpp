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

// TODO: Allow a second initiated call to test call waiting on the device.
// TODO: Spawn a task to listen for call actions from the device: outgoing
//       call, answer, hang up, reject, ...

#include <stdint.h>

#include "android/skin/qt/emulator-qt-window.h"
#include "extended-window.h"
#include "ui_extended.h"

#include <QtWidgets>
#include <QValidator>

void ExtendedWindow::initTelephony()
{
    // Set a function to validate the phone-number input
    mExtendedUi->tel_numberBox->setValidator(new phoneNumberValidator);
}

void ExtendedWindow::on_tel_startEndButton_clicked()
{
    if (mTelephonyState.mActivity == Call_Inactive) {
        // Start a call
        if (mTelephonyAgent && mTelephonyAgent->telephonyCmd) {
            // Command the emulator
            TelephonyResponse tResp;

            // Get rid of spurious characters from the phone number
            // (Allow only '+' and '0'..'9')
            // Note: phoneNumberValidator validates the user's input, but
            // allows some human-readable characters like '.' and ')'.
            // Here we remove that meaningless punctuation.
            QString cleanNumber = mExtendedUi->tel_numberBox->currentText().
                                     remove(QRegularExpression("[^+0-9]"));

            tResp = mTelephonyAgent->telephonyCmd(Tel_Op_Init_Call,
                                                  cleanNumber.toStdString().c_str());
            if (tResp != Tel_Resp_OK) {
                mToolWindow->showErrorDialog(tr("The call failed."),
                                             tr("Telephony"));
                return;
            }
        }

        // Success: Update the state and the UI buttons
        mTelephonyState.mActivity = Call_Active;
        mTelephonyState.mPhoneNumber = mExtendedUi->tel_numberBox->currentText();

        mExtendedUi->tel_numberBox->setEnabled(false);

        SettingsTheme theme = mSettingsState.mTheme;
        setButtonEnabled(mExtendedUi->tel_holdCallButton, theme, true);
        // Change the icon and text to "END CALL"
        QIcon theIcon(":/dark/call_end"); // Same for light and dark
        mExtendedUi->tel_startEndButton->setIcon(theIcon);
        mExtendedUi->tel_startEndButton->setText(tr("END CALL"));
    } else {
        // End a call
        // Update the state and the UI buttons
        mTelephonyState.mActivity = Call_Inactive;

        mExtendedUi->tel_numberBox->setEnabled(true);

        mExtendedUi->tel_holdCallButton->setProperty("themeIconName", "phone_paused");

        SettingsTheme theme = mSettingsState.mTheme;
        setButtonEnabled(mExtendedUi->tel_holdCallButton, theme, false);
        // Change the icon and text to "CALL DEVICE"
        QIcon theIcon(":/dark/phone_button"); // Same for light and dark
        mExtendedUi->tel_startEndButton->setIcon(theIcon);
        mExtendedUi->tel_startEndButton->setText(tr("CALL DEVICE"));

        if (mTelephonyAgent && mTelephonyAgent->telephonyCmd) {
            // Command the emulator
            QString cleanNumber = mExtendedUi->tel_numberBox->currentText().
                                    remove(QRegularExpression("[^+0-9]"));
            TelephonyResponse tResp;
            tResp = mTelephonyAgent->telephonyCmd(Tel_Op_Disconnect_Call,
                                                  cleanNumber.toStdString().c_str());
            if (tResp != Tel_Resp_OK) {
                mToolWindow->showErrorDialog(tr("The end-call failed."),
                                             tr("Telephony"));
                return;
            }
        }
    }
}

void ExtendedWindow::on_tel_holdCallButton_clicked()
{
    SettingsTheme theme = mSettingsState.mTheme;
    switch (mTelephonyState.mActivity) {
        case Call_Active:
            // Active --> On hold
            mTelephonyState.mActivity = Call_Held;

            mExtendedUi->tel_holdCallButton->
                setProperty("themeIconName", "phone_in_talk");

            setButtonEnabled(mExtendedUi->tel_holdCallButton,  theme, true);
            break;
        case Call_Held:
            // On hold --> Active
            mTelephonyState.mActivity = Call_Active;

            mExtendedUi->tel_holdCallButton->
                setProperty("themeIconName",          "phone_paused");
            mExtendedUi->tel_holdCallButton->
                setProperty("themeIconName_disabled", "phone_paused_disabled");

            setButtonEnabled(mExtendedUi->tel_holdCallButton,  theme, true);
            break;
        default:
            ;
    }
    if (mTelephonyAgent && mTelephonyAgent->telephonyCmd) {
        // Command the emulator
        QString cleanNumber = mExtendedUi->tel_numberBox->currentText().
                                remove(QRegularExpression("[^+0-9]"));
        TelephonyResponse  tResp;
        TelephonyOperation tOp = (mTelephonyState.mActivity == Call_Held) ?
                                     Tel_Op_Place_Call_On_Hold :
                                     Tel_Op_Take_Call_Off_Hold;

        tResp = mTelephonyAgent->telephonyCmd(tOp,
                                              cleanNumber.toStdString().c_str());
        if (tResp != Tel_Resp_OK) {
            mToolWindow->showErrorDialog(tr("The call hold failed."),
                                         tr("Telephony"));
            return;
        }
    }
}
