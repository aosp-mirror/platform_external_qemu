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

#ifndef RECORDMACROSAVEDITEM_H
#define RECORDMACROSAVEDITEM_H

#include "ui_record-macro-saved-item.h"

#include <QWidget>

namespace Ui {
class RecordMacroSavedItem;
}

class RecordMacroSavedItem : public QWidget {
    Q_OBJECT

public:
    explicit RecordMacroSavedItem(QWidget *parent = 0);

    void setName(std::string name);
    std::string getName();
    void setDisplayInfo(std::string displayInfo);

private:
    std::unique_ptr<Ui::RecordMacroSavedItem> mUi;
};

#endif // RECORDMACROSAVEDITEM_H
