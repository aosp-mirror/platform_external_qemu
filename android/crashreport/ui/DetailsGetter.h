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

#include "android/crashreport/CrashService.h"

#include <QObject>
#include <stdio.h>
#include <stdlib.h>
#include <memory>

class DetailsGetter : public QObject {
private:
    Q_OBJECT
    android::crashreport::CrashService* mCrashService;
    bool mWantHWInfo;
public:
    DetailsGetter(android::crashreport::CrashService* crashservice, bool wantHWInfo);
    std::string crash_details;

public slots:
    void getDetails();

signals:
    void finished();
    void error(QString err);
};
