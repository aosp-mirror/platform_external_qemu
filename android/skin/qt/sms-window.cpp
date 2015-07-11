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

#include <QtWidgets>
#include <QGridLayout>
#include <QComboBox>
#include <QErrorMessage>
#include <QTextEdit>
#include <QValidator>

#include "android/skin/qt/emulator-window.h"
#include "android/skin/qt/sms-window.h"
#include "telephony/android_modem.h"
#include "telephony/sms.h"
#include "telephony/modem_driver.h"

#define MAX_SMS_MSG_SIZE 1024 // Arbitrary emulator limitation

class phoneValidator : public QValidator
{
public:
    State validate(QString &input, int &pos) const;
};

SmsWindow::SmsWindow(EmulatorWindow *eW) :
    QFrame(eW),
    parentWindow(eW)
{
    Q_INIT_RESOURCE(resources);

    setWindowFlags(Qt::Tool);
    setAttribute(Qt::WA_DeleteOnClose);
    QGridLayout *layout = new QGridLayout(this);
    setLayout(layout);
    setStyleSheet(QString("QWidget { background: #2c3239 }"));

    // Text box for the message
    messageBox = new QTextEdit;
    layout->addWidget(messageBox);

    // Combo box for the "from" phone number
    phoneNumberBox = new QComboBox;
    phoneNumberBox->addItem("(650)555-1212");
    phoneNumberBox->addItem("+1.650.555-6789");
    phoneNumberBox->setCurrentIndex(0);
    phoneNumberBox->setEditable(true);
    phoneNumberBox->setInsertPolicy(QComboBox::InsertAtTop);
    phoneNumberBox->setValidator(new phoneValidator);

    layout->addWidget(phoneNumberBox);

    // "Send" button
    QPushButton *sendButton = new QPushButton(tr("Send"), this);
    QObject::connect(sendButton, &QPushButton::clicked, this, &SmsWindow::slot_sendMessage);
    layout->addWidget(sendButton);

    move(parentWindow->geometry().right(),
         ( parentWindow->geometry().top() + parentWindow->geometry().bottom() ) / 2 );

    setWindowTitle(tr("Text Messaging"));
    setMinimumSize(200, 200);
}

SmsWindow::~SmsWindow()
{
}

void SmsWindow::closeEvent(QCloseEvent *ce) {
    parentWindow->SmsWindowIsClosing();
}

void SmsWindow::slot_sendMessage()
{
    // Get the "from" number
    SmsAddressRec sender;
    int retVal = sms_address_from_str(&sender,
                                      phoneNumberBox->currentText().toStdString().c_str(),
                                      phoneNumberBox->currentText().length());
    if (retVal < 0  ||  sender.len <= 0) {
        QErrorMessage *eM = new QErrorMessage;
        eM->showMessage(QString("Invalid \"From\" number"));
        return;
    }

    // Get the text of the message
    QString theMessage = messageBox->toPlainText();

    // Convert the message text to UTF-8
    unsigned char utf8Message[MAX_SMS_MSG_SIZE+1];
    int           nUtf8Chars;
    nUtf8Chars = sms_utf8_from_message_str(theMessage.toStdString().c_str(),
                                           theMessage.length(),
                                           utf8Message,
                                           MAX_SMS_MSG_SIZE);
    if (nUtf8Chars == 0) {
        QErrorMessage *eM = new QErrorMessage;
        eM->showMessage(tr("The message is empty.\n\rPlease provide a message."));
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

// Validate the input of the "from" number.
// We allow '+' only in the first position.
// One or more digits (0-9) are required.
// Some additional characters are allowed, but ignored.

QValidator::State phoneValidator::validate(QString &input, int &pos) const
{
    int numDigits = 0;

    if (input.length() >= 32) return QValidator::Invalid;

    for (int ii=0; ii<input.length(); ii++) {
        if (input[ii] >= '0' &&  input[ii] <= '9') {
            numDigits++;
            if (numDigits > SMS_ADDRESS_MAX_SIZE) return QValidator::Invalid;
        } else if (input[ii] == '+' && ii != 0) {
            // '+' is only allowed as the first character
            return QValidator::Invalid;
        } else if (input[ii] != '-' &&
                   input[ii] != '.' &&
                   input[ii] != '(' &&
                   input[ii] != ')' &&
                   input[ii] != '/' &&
                   input[ii] != ' ' &&
                   input[ii] != '+'   )
        {
            return QValidator::Invalid;
        }
    }

    return ((numDigits > 0) ? QValidator::Acceptable : QValidator::Intermediate);
} // end validate()
