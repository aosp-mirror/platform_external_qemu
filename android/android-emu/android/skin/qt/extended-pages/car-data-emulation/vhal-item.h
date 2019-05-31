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
#ifndef VHALITEM_H
#define VHALITEM_H

#include <QWidget>

namespace Ui {
class VhalItem;
}

class VhalItem : public QWidget {
    Q_OBJECT

public:
    explicit VhalItem(QWidget* parent = nullptr,
                      QString name = "null",
                      QString proId = "0");
    ~VhalItem();

    void vhalItemSelected(bool state);

private:
    Ui::VhalItem* ui;
};

#endif  // VHALITEM_H
