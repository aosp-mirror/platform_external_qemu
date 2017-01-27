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

#include "android/skin/qt/extended-pages/google-play-page.h"

GooglePlayPage::GooglePlayPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::GooglePlayPage)
{
    mUi->setupUi(this);
}

GooglePlayPage::~GooglePlayPage() {
}

void GooglePlayPage::on_goog_updateServicesButton_clicked() {
}
