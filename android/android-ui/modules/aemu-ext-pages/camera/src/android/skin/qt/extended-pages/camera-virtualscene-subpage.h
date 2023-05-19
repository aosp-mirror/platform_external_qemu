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

#include <QObject>                           // for Q_OBJECT, slots
#include <QString>                           // for QString
#include <QWidget>                           // for QWidget
#include <memory>                            // for shared_ptr, unique_ptr

#include "ui_camera-virtualscene-subpage.h"  // for CameraVirtualSceneSubpage

class QObject;
class QShowEvent;
struct QAndroidVirtualSceneAgent;


namespace android {
namespace metrics {
class UiEventTracker;
}  // namespace metrics
}  // namespace android

using android::metrics::UiEventTracker;
class CameraVirtualSceneSubpage : public QWidget {
    Q_OBJECT

public:
    explicit CameraVirtualSceneSubpage(QWidget* parent = 0);

    static void setVirtualSceneAgent(const QAndroidVirtualSceneAgent* agent);

    void showEvent(QShowEvent* event) override;

private slots:
    void on_imageWall_pathChanged(QString path);
    void on_imageTable_pathChanged(QString path);

    void on_imageWall_scaleChanged(float value);
    void on_imageTable_scaleChanged(float value);

    void on_toggleTV_toggled(bool value);

    // Report metrics for the first interaction to this page.
    void reportInteraction();

private:
    void changePoster(QString name, QString path);
    void changePosterScale(QString name, float value);
    void loadUi();

    // Load initial settings to populate the virtual scene during emulator
    // startup.
    static void loadInitialSettings();

    std::unique_ptr<Ui::CameraVirtualSceneSubpage> mUi;
    std::shared_ptr<UiEventTracker> mCameraTracker;
    bool mHasBeenShown = false;
    bool mHadFirstInteraction = false;

    static const QAndroidVirtualSceneAgent* sVirtualSceneAgent;
};
