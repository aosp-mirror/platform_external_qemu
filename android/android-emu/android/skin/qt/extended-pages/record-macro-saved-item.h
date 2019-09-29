// Copyright (C) 2019 The Android Open Source Project
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

#include <qobjectdefs.h>                                   // for Q_OBJECT
#include <QString>                                         // for QString
#include <string>                                          // for string

#include "android/skin/qt/common-controls/cc-list-item.h"  // for CCListItem

class QObject;
class QString;
class QWidget;

class RecordMacroSavedItem : public CCListItem {
    Q_OBJECT

public:
    explicit RecordMacroSavedItem(QWidget* parent = 0);

    void setName(QString name);
    std::string getName() const;
    void setDisplayInfo(QString displayInfo);
    void setDisplayTime(QString displayTime);
    bool isPreset();
    void setIsPreset(bool isPreset);

private:
    bool mIsPreset = false;
};
