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

#include "ui_record-and-playback-page.h"

#include <QWidget>
#include <QTabBar>
#include <memory>

class RecordAndPlaybackPage : public QWidget {
    Q_OBJECT

public:
    explicit RecordAndPlaybackPage(QWidget* parent = 0);

    void updateTheme();

    std::unique_ptr<Ui::RecordAndPlaybackPage> mUi;
    QTabBar* mTabBar = nullptr;
};
