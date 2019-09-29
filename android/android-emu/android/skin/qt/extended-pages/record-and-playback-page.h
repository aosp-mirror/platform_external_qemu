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

#include <qobjectdefs.h>                  // for Q_OBJECT, signals, slots
#include <QString>                        // for QString
#include <QWidget>                        // for QWidget
#include <memory>                         // for unique_ptr

#include "ui_record-and-playback-page.h"  // for RecordAndPlaybackPage

class QObject;
class QTabBar;
class QWidget;

class RecordAndPlaybackPage : public QWidget {
    Q_OBJECT

public:
    explicit RecordAndPlaybackPage(QWidget* parent = 0);

    void updateTheme();
    void enableCustomMacros();
    void removeSettingsTab();
    void focusMacroRecordTab();

    std::unique_ptr<Ui::RecordAndPlaybackPage> mUi;
    QTabBar* mTabBar = nullptr;

signals:
    void setRecordingStateSignal(bool state);
    void ensureVirtualSceneWindowCreated();

private slots:
    void setRecordingStateSlot(bool state);
};
