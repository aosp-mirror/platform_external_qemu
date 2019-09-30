// Copyright 2018 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "android/skin/qt/QtThreading.h"

#include <assert.h>          // for assert
#include <QApplication>      // for QApplication
#include <QCoreApplication>  // for QCoreApplication
#include <QObject>           // for QObject
#include <QThread>           // for QThread

class QObject;

namespace android {
namespace qt {

void moveToMainThread(QObject* object) {
    const auto mainThread = QApplication::instance()->thread();
    assert(mainThread);
    if (QThread::currentThread() != mainThread) {
        object->moveToThread(mainThread);
    }
}

}  // namespace qt
}  // namespace android
