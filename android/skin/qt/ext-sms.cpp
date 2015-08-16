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

#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include "android/skin/qt/emulator-qt-window.h"
#include "android/telephony/modem.h"
#include "android/telephony/sms.h"
#include "extended-window.h"
#include "telephony/modem_driver.h"
#include "ui_extended.h"

#include <QtWidgets>
#include <QValidator>

#define MAX_SMS_MSG_SIZE 1024 // Arbitrary emulator limitation

void ExtendedWindow::initSms()
{
    // Set a function to validate the phone-number input
    mExtendedUi->sms_phoneNumberBox->setValidator(new phoneNumberValidator);
}

void ExtendedWindow::on_sms_sendButton_clicked()
{
    // Get the "from" number
    SmsAddressRec sender;
    int retVal = sms_address_from_str(&sender,
                                      mExtendedUi->sms_phoneNumberBox->currentText().toStdString().c_str(),
                                      mExtendedUi->sms_phoneNumberBox->currentText().length());
    if (retVal < 0  ||  sender.len <= 0) {
        QErrorMessage *eM = new QErrorMessage;
        eM->showMessage(QString("The \"From\" number is invalid"));
        return;
    }

    // Get the text of the message
    QString theMessage = mExtendedUi->sms_messageBox->toPlainText();

    // Convert the message text to UTF-8
    unsigned char utf8Message[MAX_SMS_MSG_SIZE+1];
    int           nUtf8Chars;
    nUtf8Chars = sms_utf8_from_message_str(theMessage.toStdString().c_str(),
                                           theMessage.length(),
                                           utf8Message,
                                           MAX_SMS_MSG_SIZE);
    if (nUtf8Chars == 0) {
        QErrorMessage *eM = new QErrorMessage;
        eM->showMessage(tr("The message is empty.<br>Please enter a message."));
        return;
    }

    if (nUtf8Chars < 0) {
        QErrorMessage *eM = new QErrorMessage;
        eM->showMessage(QString(tr("The message contains invalid characters")));
        return;
    }

    // Create a list of SMS PDUs, then send them
    SmsPDU *pdus = smspdu_create_deliver_utf8(utf8Message, nUtf8Chars, &sender, NULL);
    if (pdus == NULL) {
        QErrorMessage *eM = new QErrorMessage;
        eM->showMessage(QString(tr("The message contains invalid characters")));
        return;
    }

    for (int idx = 0; pdus[idx] != NULL; idx++)
        amodem_receive_sms( android_modem, pdus[idx] );

    smspdu_free_list( pdus );
}
