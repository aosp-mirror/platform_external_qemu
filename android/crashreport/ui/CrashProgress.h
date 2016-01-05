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

#include <QProgressDialog>
#include <QObject>

class CrashProgress : public QDialog {
private:
    Q_OBJECT
    QProgressDialog mProgressDialog;
public:
    CrashProgress(QObject* parent);
public slots:
    void start();
    void stop();
};
