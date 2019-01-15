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

#include "android/avd/BugreportInfo.h"
#include "android/skin/qt/qt-ui-commands.h"
#include "android/skin/qt/shortcut-key-store.h"

#include "ui_help-page.h"
#include <QWidget>
#include <memory>


class HelpPage : public QWidget
{
    Q_OBJECT

public:
    explicit HelpPage(QWidget *parent = 0);
    void initialize(const ShortcutKeyStore<QtUICommand>* key_store);

private slots:
    void on_help_docs_clicked();
    void on_help_sendFeedback_clicked();

private:
    void initializeLicenseText();
    void initializeKeyboardShortcutList(const ShortcutKeyStore<QtUICommand>* key_store);

    std::unique_ptr<Ui::HelpPage> mUi;
    android::avd::BugreportInfo mBugreportInfo;
};

class LatestVersionLoadTask : public QObject {
    Q_OBJECT

public slots:
    void run();

signals:
    void finished(QString version);
};
