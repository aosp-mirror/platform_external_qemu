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

#include "ui_cc-list-item.h"

#include "android/skin/qt/common-controls/cc-layout-direction.h"

#include <QWidget>
#include <memory>

namespace Ui {
class CCListItem;
}

class CCListItem : public QWidget {
    Q_OBJECT

public:
    explicit CCListItem(QWidget* parent = 0);

    void setTitle(QString title);
    std::string getTitle() const;
    void setSubtitle(QString displayInfo);
    // Only support CCLayoutDirection Left and Right
    void setLabelText(QString extraText, CCLayoutDirection direction = CCLayoutDirection::Right);
    void setLabelPixmap(QPixmap pixmap, CCLayoutDirection direction = CCLayoutDirection::Right);
    void setLabelSize(int width, int height, CCLayoutDirection direction = CCLayoutDirection::Right);
    void setEditButtonEnabled(bool enabled);
    void setSelected(bool selected);
    bool isSelected() const;
    void updateTheme();

private:
    void setSubtitleOpacity(double opacity);

private slots:
    void on_editButton_clicked();

signals:
    void editButtonClickedSignal(CCListItem* item);

private:
    std::unique_ptr<Ui::CCListItem> mUi;
    bool mSelected = false;
};
