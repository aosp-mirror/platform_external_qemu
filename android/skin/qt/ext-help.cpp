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

#include "android/skin/qt/extended-window.h"

#include <QDesktopServices>
#include <QUrl>

const QString DOCS_URL =
    "http://developer.android.com/tools/help/emulator.html";
const QString FILE_BUG_URL =
    "https://code.google.com/p/android/issues/entry?template=Android%20Emulator%20Bug";
const QString SEND_FEEDBACK_URL =
    "https://code.google.com/p/android/issues/entry?template=Emulator%20Feature%20Request";

void ExtendedWindow::on_help_docs_clicked() {
    QDesktopServices::openUrl(QUrl(DOCS_URL));
}

void ExtendedWindow::on_help_fileBug_clicked() {
    QDesktopServices::openUrl(QUrl(FILE_BUG_URL));
}

void ExtendedWindow::on_help_sendFeedback_clicked() {
    QDesktopServices::openUrl(QUrl(SEND_FEEDBACK_URL));
}

