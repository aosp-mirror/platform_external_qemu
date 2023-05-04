// Copyright (C) 2017 The Android Open Source Project
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

#include <qobjectdefs.h>       // for Q_OBJECT
#include <QString>             // for QString
#include <QWidget>             // for QWidget
#include <memory>              // for unique_ptr
#include <string>              // for string

#include "ui_car-data-page.h"  // for CarDataPage

class QObject;
class QString;
class QWidget;
struct QCarDataAgent;

namespace emulator {
class EmulatorMessage;
}

class CarDataPage : public QWidget {
    Q_OBJECT
public:
    explicit CarDataPage(QWidget* parent = nullptr);
    static void carDataCallback(const char* msg, int length, void* context);
    static void setCarDataAgent(const QCarDataAgent* agent);
    void onReceiveData(const char* msg, int length);
    void sendCarEmulatorMessageLogged(const emulator::EmulatorMessage& msg,
                                      const std::string& log);

private:
    static const QCarDataAgent* sCarDataAgent;
    static const int DATA_PROPERTY_TABLE_INDEX = 1;

    std::unique_ptr<Ui::CarDataPage> mUi;
    void updateReceivedData(const QString msg);
};
