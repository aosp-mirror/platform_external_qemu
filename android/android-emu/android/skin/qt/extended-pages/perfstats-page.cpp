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
#include "android/skin/qt/extended-pages/perfstats-page.h"

PerfStatsPage::PerfStatsPage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::PerfStatsPage()) {
    mUi->setupUi(this);

    connect(this, SIGNAL(windowVisible()),
            this, SLOT(enableCollection()));

    connect(this, SIGNAL(windowHidden()),
            this, SLOT(disableCollection()));
}

PerfStatsPage::~PerfStatsPage() { }

void PerfStatsPage::showEvent(QShowEvent* event) {
    emit windowVisible();
}

void PerfStatsPage::hideEvent(QHideEvent* event) {
    emit windowHidden();
}

void PerfStatsPage::enableCollection() {
    mUi->perfstatsWidget->onCollectionEnabled();
}

void PerfStatsPage::disableCollection() {
    mUi->perfstatsWidget->onCollectionDisabled();
}

void PerfStatsPage::updateAndRefreshValues() {
    mUi->perfstatsWidget->updateAndRefreshValues();
}