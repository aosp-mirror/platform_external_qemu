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

#ifndef SMS_WINDOW_H
#define SMS_WINDOW_H

#include <QComboBox>
#include <QFrame>
//#include <QListWidget>
#include <QTextEdit>

class EmulatorWindow;
class SmsWindow;
typedef void(SmsWindow::*SmsWindowSlot)();

namespace Ui {
    class SmsWindow;
}

class SmsWindow : public QFrame
{
    Q_OBJECT

public:
    explicit SmsWindow(EmulatorWindow *eW);

private:

    ~SmsWindow();
    void closeEvent(QCloseEvent *ce);

    EmulatorWindow *parentWindow;
    QTextEdit      *messageBox;
    QComboBox      *phoneNumberBox;

private slots:
    void slot_sendMessage();
};

#endif // SMS_WINDOW_H
