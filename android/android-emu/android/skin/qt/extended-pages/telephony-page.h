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

#pragma once

#include <qcoreevent.h>   // for QEvent (ptr only), QEvent::Type
#include <qobjectdefs.h>  // for Q_OBJECT, slots
#include <qvalidator.h>   // for QValidator::State
#include <QEvent>         // for QEvent
#include <QString>        // for QString
#include <QValidator>     // for QValidator
#include <QWidget>        // for QWidget
#include <memory>         // for unique_ptr

class QObject;
class QString;
class QWidget;
namespace Ui {
class TelephonyPage;
}  // namespace Ui
struct QAndroidTelephonyAgent;

class TelephonyPage : public QWidget
{
    Q_OBJECT

public:
    explicit TelephonyPage(QWidget *parent = 0);
    ~TelephonyPage();

    static void setTelephonyAgent(const QAndroidTelephonyAgent* agent);
    void eventLauncher(int);

private slots:
    void on_tel_startEndButton_clicked();
    void on_tel_holdCallButton_clicked();
    void on_sms_sendButton_clicked();

    void customEvent(QEvent*);

private:
    class PhoneNumberValidator : public QValidator
    {
    public:
        // Validate the input of a telephone number.
        // We allow '+' only in the first position.
        // One or more digits (0-9) are required.
        // Some additional characters are allowed, but ignored.
        State validate(QString &input, int &pos) const;

    private:
        static State validateAsDigital(const QString &input);
        static State validateAsAlphanumeric(const QString &input);
    };

    enum class CallActivity {
        Inactive, Active, Held
    };

    std::unique_ptr<Ui::TelephonyPage> mUi;
    CallActivity mCallActivity;
    QString mPhoneNumber;
    QEvent::Type mCustomEventType;
};
