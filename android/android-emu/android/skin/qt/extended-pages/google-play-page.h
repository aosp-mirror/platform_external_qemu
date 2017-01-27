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

#include "ui_google-play-page.h"
#include <QWidget>
#include <memory>

class GooglePlayPage : public QWidget
{
    Q_OBJECT
public:
    explicit GooglePlayPage(QWidget *parent = 0);
    ~GooglePlayPage();

private slots:
    void on_goog_updateServicesButton_clicked();
    void on_goog_updateStoreButton_clicked();

private:
    std::unique_ptr<Ui::GooglePlayPage> mUi;
};
