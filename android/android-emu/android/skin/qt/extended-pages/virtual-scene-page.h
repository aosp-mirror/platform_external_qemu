// Copyright (C) 2018 The Android Open Source Project
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

#include "ui_virtual-scene-page.h"

#include <QWidget>
#include <memory>

struct QAndroidVirtualSceneAgent;

class VirtualScenePage : public QWidget {
    Q_OBJECT

public:
    explicit VirtualScenePage(QWidget* parent = 0);

    static void setVirtualSceneAgent(const QAndroidVirtualSceneAgent* agent);

    void showEvent(QShowEvent* event) override;

private slots:
    void on_imageWall_pathChanged(QString path);
    void on_imageTable_pathChanged(QString path);

    void on_sizeSliderWall_valueChanged(double value);
    void on_sizeSliderTable_valueChanged(double value);

    // Report metrics for the first interaction to this page.
    void reportInteraction();

private:
    void changePoster(QString name, QString path);
    void changePosterSize(QString name, float value);
    void loadUi();

    // Load initial settings to populate the virtual scene during emulator
    // startup.
    static void loadInitialSettings();

    std::unique_ptr<Ui::VirtualScenePage> mUi;
    bool mHasBeenShown = false;
    bool mHadFirstInteraction = false;

    static const QAndroidVirtualSceneAgent* sVirtualSceneAgent;
};
