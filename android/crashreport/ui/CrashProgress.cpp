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

#include "android/crashreport/ui/CrashProgress.h"

CrashProgress::CrashProgress(QObject* parent) {
    mProgressDialog.setWindowTitle(tr("Android Emulator"));
    mProgressDialog.setLabelText(tr("Collecting system-specific info..."));
    mProgressDialog.setRange(0, 0);
    mProgressDialog.setCancelButton(0);
}

void CrashProgress::start() { mProgressDialog.show(); }
void CrashProgress::stop() { mProgressDialog.close(); }

