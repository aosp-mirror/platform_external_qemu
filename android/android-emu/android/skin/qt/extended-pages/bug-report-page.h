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

#include "ui_bug-report-page.h"

#include <QFrame>
#include <memory>

class BugReportPage : public QFrame {
    Q_OBJECT

public:
    explicit BugReportPage(QWidget* parent = 0);

private:
    std::unique_ptr<Ui::BugReportPage> mUi;
};
