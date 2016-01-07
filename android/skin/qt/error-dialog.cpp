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

#include "android/skin/qt/error-dialog.h"
#include <QErrorMessage>

void showErrorDialog(const QString& message, const QString& title)
{
    QErrorMessage *err = QErrorMessage::qtHandler();
    err->setWindowTitle(title);
    err->showMessage(message);
}
