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

#include <qobjectdefs.h>                     // for Q_OBJECT, slots, signals
#include <QObject>                           // for QObject
#include <QString>                           // for QString
#include <QWidget>                           // for QWidget
#include <memory>                            // for unique_ptr

#include "android/avd/BugreportInfo.h"       // for BugreportInfo
#include "android/skin/qt/qt-ui-commands.h"  // for QtUICommand
#include "ui_help-page.h"                    // for HelpPage


namespace android {
namespace metrics {
class UiEventTracker;
}  // namespace metrics
}  // namespace android

using android::metrics::UiEventTracker;
class QObject;
class QString;
class QWidget;
template <class CommandType> class ShortcutKeyStore;


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
    void disableForEmbeddedEmulator();

    std::unique_ptr<Ui::HelpPage> mUi;
    std::shared_ptr<UiEventTracker> mHelpTracker;
    android::avd::BugreportInfo mBugreportInfo;
};

class LatestVersionLoadTask : public QObject {
    Q_OBJECT

public slots:
    void run();

signals:
    void finished(QString version);
};
