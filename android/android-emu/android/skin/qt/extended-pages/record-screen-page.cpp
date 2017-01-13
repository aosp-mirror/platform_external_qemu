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

#include "android/skin/qt/extended-pages/record-screen-page.h"

//#include "android/emulation/control/record-screen_agent.h"

RecordScreenPage::RecordScreenPage(QWidget *parent) :
    QWidget(parent),
    mUi(new Ui::RecordScreenPage)
{
    mUi->setupUi(this);
}

RecordScreenPage::~RecordScreenPage() {
}
