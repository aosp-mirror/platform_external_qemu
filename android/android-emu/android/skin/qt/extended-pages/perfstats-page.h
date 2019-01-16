// Copyright (C) 2015 The Android Open Source Project
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

#include "ui_perfstats-page.h"

#include <QTimer>

#include <memory>

class PerfStatsPage : public QWidget
{
    Q_OBJECT

public:
    explicit PerfStatsPage(QWidget* parent = 0);
    ~PerfStatsPage();

    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:

signals:

    void windowVisible();
    void windowHidden();

public slots:

void enableCollection();
void disableCollection();
void updateAndRefreshValues();

private slots:

private:

    QTimer mCollectionTimer;
    std::unique_ptr<Ui::PerfStatsPage> mUi;
};
