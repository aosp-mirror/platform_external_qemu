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

#include "android/emulation/control/utils/SimpleVirtualSceneClient.h"
#include "ui_camera-virtualscene-subpage-grpc.h"  // for CameraVirtualSceneSubpageGrpc


namespace android {
namespace metrics {
class UiEventTracker;
}  // namespace metrics
}  // namespace android

using android::emulation::control::SimpleVirtualSceneServiceClient;


using android::metrics::UiEventTracker;
class CameraVirtualSceneSubpageGrpc : public QWidget {
    Q_OBJECT

public:
    explicit CameraVirtualSceneSubpageGrpc(QWidget* parent = 0);
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
    void loadInitialSettings();

    std::unique_ptr<Ui::CameraVirtualSceneSubpageGrpc> mUi;
    std::shared_ptr<UiEventTracker> mCameraTracker;
    bool mHasBeenShown = false;
    bool mHadFirstInteraction = false;
    SimpleVirtualSceneServiceClient mServiceClient;
};