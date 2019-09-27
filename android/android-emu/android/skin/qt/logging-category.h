/* Copyright (C) 2019 The Android Open Source Project
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

#pragma once

#include <QLoggingCategory>

// The purpose for this is to silence the qWarning() and others that come from the Qt framework.
// In order to silence, for example, all qWarning() calls, we use an environment variable and set it as:
// QT_LOGGING_RULES="default.warning=false". For qDebug(), default.debug=false.
//
// The issue with setting the environment variable is that even our own usage will be silenced. To mitigate
// this, we can make our "category", called |emu|, similar to android's TAG name, and pass it into the logging
// system. So now, logging will look like:
//
// qCDebug(emu) << "Hello world!";
// qCWarning(emu) << "Hello world!";
//
// Q_DECLARE_LOGGING_CATEGORY create a forward declaration of a logging function, and the real definition is
// in winsys_qt.cpp, and is declared using the Q_LOGGING_FACTORY() macro.
Q_DECLARE_LOGGING_CATEGORY(emu)
