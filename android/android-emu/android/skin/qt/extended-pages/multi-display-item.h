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

#include "ui_multi-display-item.h"
#include <QWidget>
#include <memory>

class MultiDisplayItem : public QWidget {
    Q_OBJECT

public:
    explicit MultiDisplayItem(int id, QWidget* parent = 0);
private:
    std::unique_ptr<Ui::MultiDisplayItem> mUi;
    struct displayType  {
        std::string name;
        int height;
        int width;
        uint32_t dpi;
    };
    static std::vector<displayType> sDisplayTypes;
    int mId;
    int mCurrentIndex;
    void hideWidthHeightDpiBox(bool hide);
    void getValues(int* width, int* height, uint32_t* dpi);
    void setValues(int index);

private slots:
    void onDisplayTypeChanged(int index);
    void on_deleteDisplay_clicked();

signals:
    void deleteDisplay(int id);
};

