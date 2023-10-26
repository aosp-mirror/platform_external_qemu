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

#include "android/skin/qt/extended-pages/camera-virtualscene-subpage-grpc.h"

#include <QByteArray>    // for QByteArray
#include <QCheckBox>     // for QCheckBox
#include <QFileInfo>     // for QFileInfo
#include <QMap>          // for QMap
#include <QMapIterator>  // for QMapIterator
#include <QSettings>     // for QSettings
#include <QVariant>      // for QVariant
#include <functional>    // for __base

#include "android/avd/util.h"                               // for path_getA...
#include "android/console.h"                                // for android_hw
#include "android/emulation/control/virtual_scene_agent.h"  // for QAndroidV...
#include "android/metrics/MetricsReporter.h"                // for MetricsRe...
#include "android/metrics/UiEventTracker.h"                 // for UiEventTr...
#include "android/skin/qt/function-runner.h"
#include "android/skin/qt/poster-image-well.h"  // for PosterIma...
#include "android/skin/qt/qt-settings.h"        // for PER_AVD_S...
#include "host-common/hw-config.h"
#include "studio_stats.pb.h"  // for EmulatorU...

using android::emulation::control::EmulatorGrpcClient;
using android::emulation::control::incubating::AnimationState;
using android::emulation::control::incubating::Poster;
using android::emulation::control::incubating::PosterList;

CameraVirtualSceneSubpageGrpc::CameraVirtualSceneSubpageGrpc(QWidget* parent)
    : QWidget(parent),
      mUi(new Ui::CameraVirtualSceneSubpageGrpc()),
      mCameraTracker(new UiEventTracker(
              android_studio::EmulatorUiEvent::BUTTON_PRESS,
              android_studio::EmulatorUiEvent::EXTENDED_CAMERA_TAB)),
      mServiceClient(EmulatorGrpcClient::me()) {
    mUi->setupUi(this);

    connect(mUi->imageWall, SIGNAL(interaction()), this,
            SLOT(reportInteraction()));
    connect(mUi->imageTable, SIGNAL(interaction()), this,
            SLOT(reportInteraction()));
}

void CameraVirtualSceneSubpageGrpc::showEvent(QShowEvent* event) {
    if (!mHasBeenShown) {
        mHasBeenShown = true;
        loadUi();
    }
}

void CameraVirtualSceneSubpageGrpc::on_imageWall_pathChanged(QString path) {
    mCameraTracker->increment("WALL");
    changePoster("wall", path);
}

void CameraVirtualSceneSubpageGrpc::on_imageWall_scaleChanged(float value) {
    changePosterScale("wall", value);
}

void CameraVirtualSceneSubpageGrpc::on_imageTable_pathChanged(QString path) {
    mCameraTracker->increment("TABLE");
    changePoster("table", path);
}

void CameraVirtualSceneSubpageGrpc::on_imageTable_scaleChanged(float value) {
    changePosterScale("table", value);
}

void CameraVirtualSceneSubpageGrpc::on_toggleTV_toggled(bool value) {
    // Persist to settings.
    const char* avdPath = path_getAvdContentPath(
            getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        const QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);

        avdSpecificSettings.setValue(
                Ui::Settings::PER_AVD_VIRTUAL_SCENE_TV_ANIMATION, value);
    }

    // Update the scene.
    AnimationState animation;
    animation.set_tvon(value);
    mServiceClient.setAnimationStateAsync(animation, [](auto ignored) {});
}

void CameraVirtualSceneSubpageGrpc::reportInteraction() {
    if (!mHadFirstInteraction) {
        mHadFirstInteraction = true;
        android::metrics::MetricsReporter::get().report(
                [](android_studio::AndroidStudioEvent* event) {
                    event->mutable_emulator_details()
                            ->mutable_used_features()
                            ->set_virtualscene_config(true);
                });
    }
}

void CameraVirtualSceneSubpageGrpc::changePoster(QString name, QString path) {
    // Persist to settings.
    const char* avdPath = path_getAvdContentPath(
            getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        const QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);

        QMap<QString, QVariant> savedPosters =
                avdSpecificSettings
                        .value(Ui::Settings::PER_AVD_VIRTUAL_SCENE_POSTERS)
                        .toMap();
        savedPosters[name] = path;

        avdSpecificSettings.setValue(
                Ui::Settings::PER_AVD_VIRTUAL_SCENE_POSTERS, savedPosters);
    }

    // Update the ImageWells so that future file selections will use this file's
    // directory as their starting directory.
    if (!path.isNull()) {
        QString dir = QFileInfo(path).path();
        mUi->imageWall->setStartingDirectory(dir);
        mUi->imageTable->setStartingDirectory(dir);
    }

    Poster poster;
    poster.set_name(name.toStdString());
    if (path.isNull()) {
        poster.mutable_file_name()->set_value(path.toStdString());
    }

    // Update the scene.
    mServiceClient.setPosterAsync(poster, [](auto ignored) {});
}

void CameraVirtualSceneSubpageGrpc::changePosterScale(QString name,
                                                      float value) {
    // Persist to settings.
    const char* avdPath = path_getAvdContentPath(
            getConsoleAgents()->settings->hw()->avd_name);
    if (avdPath) {
        const QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);

        QMap<QString, QVariant> savedPosterSizes =
                avdSpecificSettings
                        .value(Ui::Settings::PER_AVD_VIRTUAL_SCENE_POSTER_SIZES)
                        .toMap();
        savedPosterSizes[name] = value;

        avdSpecificSettings.setValue(
                Ui::Settings::PER_AVD_VIRTUAL_SCENE_POSTER_SIZES,
                savedPosterSizes);
    }

    // Update the scene.
    Poster poster;
    poster.set_name(name.toStdString());
    poster.set_scale(value);
    mServiceClient.setPosterAsync(poster, [](auto ignrored) {});
}

void CameraVirtualSceneSubpageGrpc::loadUi() {
    // Enumerate existing pointers.  If any are currently set, they were set
    // from the command line so they take precedence over the saved setting.
    mServiceClient.listPostersAsync(
            [this](absl::StatusOr<PosterList*> posters) {
                if (!posters.ok()) {
                    return;
                }
                for (const auto& poster : posters.value()->posters()) {
                    if (poster.name() == "wall") {
                        runOnEmuUiThread([this, poster] {
                            mUi->imageWall->setPath(
                                    poster.file_name().value().c_str());
                            mUi->imageWall->setScale(poster.scale());
                            mUi->imageWall->setMinMaxSize(poster.minwidth(),
                                                          poster.maxwidth());
                        });
                    }

                    if (poster.name() == "table") {
                        runOnEmuUiThread([this, poster] {
                            mUi->imageTable->setPath(
                                    poster.file_name().value().c_str());
                            mUi->imageTable->setScale(poster.scale());
                            mUi->imageTable->setMinMaxSize(poster.minwidth(),
                                                           poster.maxwidth());
                        });
                    }
                }
            });

    mServiceClient.getAnimationStateAsync(
            [this](absl::StatusOr<AnimationState*> animation) {
                if (!animation.ok()) {
                    return;
                }
                // Set UI display to correct state.
                mUi->toggleTV->setChecked(animation.value()->tvon());
            });
}

// static
void CameraVirtualSceneSubpageGrpc::loadInitialSettings() {
    const char* avdPath = path_getAvdContentPath(
            getConsoleAgents()->settings->hw()->avd_name);
    if (!avdPath)
        return;

    const QString avdSettingsFile =
            avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);

    mServiceClient.listPostersAsync([avdSettingsFile,
                                     this](absl::StatusOr<PosterList*>
                                                   posters) {
        std::unordered_set<std::string> alreadyLoaded;
        if (posters.ok()) {
            for (const auto& poster : posters.value()->posters()) {
                alreadyLoaded.insert(poster.name());
            }
        }

        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);

        const QMap<QString, QVariant> savedPosters =
                avdSpecificSettings
                        .value(Ui::Settings::PER_AVD_VIRTUAL_SCENE_POSTERS)
                        .toMap();
        const QMap<QString, QVariant> savedPosterSizes =
                avdSpecificSettings
                        .value(Ui::Settings::PER_AVD_VIRTUAL_SCENE_POSTER_SIZES)
                        .toMap();
        const bool savedAnimationState =
                avdSpecificSettings
                        .value(Ui::Settings::PER_AVD_VIRTUAL_SCENE_TV_ANIMATION)
                        .toBool();

        // Send the saved posters to the virtual scene.  This is called
        // early during emulator startup.  If a poster has been set via
        // a command line option, that poster will take priority and
        // setInitialSetting will not replace it.
        QMapIterator<QString, QVariant> it(savedPosters);
        while (it.hasNext()) {
            it.next();

            // Already loaded, so just skip it.
            const QString& name = it.key();
            if (alreadyLoaded.count(name.toStdString())) {
                continue;
            }

            const QString path = it.value().toString();
            Poster poster;
            poster.set_name(name.toStdString());
            poster.mutable_file_name()->set_value(path.toStdString());

            if (savedPosterSizes.contains(name)) {
                bool valid = false;
                const float size = savedPosterSizes[name].toFloat(&valid);
                if (valid) {
                    poster.set_scale(size);
                }
            }

            mServiceClient.setPosterAsync(poster, [](auto ignored) {});
        }
    });

    QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);
    const bool savedAnimationState =
            avdSpecificSettings
                    .value(Ui::Settings::PER_AVD_VIRTUAL_SCENE_TV_ANIMATION)
                    .toBool();

    // Set animation state.
    AnimationState animation;
    animation.set_tvon(savedAnimationState);
    mServiceClient.setAnimationStateAsync(animation, [](auto ignored) {});
}