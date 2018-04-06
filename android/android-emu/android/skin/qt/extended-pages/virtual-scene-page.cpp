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

#include "android/skin/qt/extended-pages/virtual-scene-page.h"

#include "android/emulation/control/virtual_scene_agent.h"
#include "android/globals.h"
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/proto/studio_stats.pb.h"
#include "android/skin/qt/qt-settings.h"

#include <QDebug>
#include <QFileInfo>
#include <QSettings>

static constexpr float kSliderValueScale = 100.0f;

const QAndroidVirtualSceneAgent* VirtualScenePage::sVirtualSceneAgent = nullptr;

VirtualScenePage::VirtualScenePage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::VirtualScenePage()) {
    mUi->setupUi(this);

    mUi->sizeSliderWall->setRange(0.0, kSliderValueScale, false);
    mUi->sizeSliderWall->setValue(kSliderValueScale, false);
    mUi->sizeSliderTable->setRange(0.0, kSliderValueScale, false);
    mUi->sizeSliderTable->setValue(kSliderValueScale, false);

    connect(mUi->imageWall, SIGNAL(interaction()), this,
            SLOT(reportInteraction()));
    connect(mUi->imageTable, SIGNAL(interaction()), this,
            SLOT(reportInteraction()));
}

// static
void VirtualScenePage::setVirtualSceneAgent(
        const QAndroidVirtualSceneAgent* agent) {
    sVirtualSceneAgent = agent;
    loadInitialSettings();
}

void VirtualScenePage::showEvent(QShowEvent* event) {
    if (!mHasBeenShown) {
        mHasBeenShown = true;
        loadUi();
    }
}

void VirtualScenePage::on_imageWall_pathChanged(QString path) {
    changePoster("wall", path);
}

void VirtualScenePage::on_imageTable_pathChanged(QString path) {
    changePoster("table", path);
}

void VirtualScenePage::on_sizeSliderWall_valueChanged(double value) {
    changePosterSize("wall", static_cast<float>(value));
}

void VirtualScenePage::on_sizeSliderTable_valueChanged(double value) {
    changePosterSize("table", static_cast<float>(value));
}

void VirtualScenePage::reportInteraction() {
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

void VirtualScenePage::changePoster(QString name, QString path) {
    // Persist to settings.
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
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

    // Update the scene.
    if (sVirtualSceneAgent) {
        sVirtualSceneAgent->loadPoster(
                name.toUtf8().constData(),
                path.isNull() ? nullptr : path.toUtf8().constData());
    }
}

void VirtualScenePage::changePosterSize(QString name, float value) {
    const float scaledValue = value / kSliderValueScale;

    // Persist to settings.
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        const QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);

        QMap<QString, QVariant> savedPosterSizes =
                avdSpecificSettings
                        .value(Ui::Settings::PER_AVD_VIRTUAL_SCENE_POSTER_SIZES)
                        .toMap();
        savedPosterSizes[name] = scaledValue;

        avdSpecificSettings.setValue(
                Ui::Settings::PER_AVD_VIRTUAL_SCENE_POSTER_SIZES,
                savedPosterSizes);
    }

    // Update the scene.
    if (sVirtualSceneAgent) {
        sVirtualSceneAgent->setPosterSize(name.toUtf8().constData(),
                                          scaledValue);
    }
}

void VirtualScenePage::loadUi() {
    // Enumerate existing pointers.  If any are currently set, they were set
    // from the command line so they take precedence over the saved setting.
    sVirtualSceneAgent->enumeratePosters(
            this, [](void* context, const char* posterName,
                     const char* filename, float size) {
                auto self = reinterpret_cast<VirtualScenePage*>(context);
                const QString name = posterName;
                if (name == "wall") {
                    self->mUi->imageWall->setPath(filename);
                    self->mUi->sizeSliderWall->setValue(
                            size * kSliderValueScale, false);
                } else if (name == "table") {
                    self->mUi->imageTable->setPath(filename);
                    self->mUi->sizeSliderTable->setValue(
                            size * kSliderValueScale, false);
                }
            });
}

// static
void VirtualScenePage::loadInitialSettings() {
    const char* avdPath = path_getAvdContentPath(android_hw->avd_name);
    if (avdPath) {
        const QString avdSettingsFile =
                avdPath + QString(Ui::Settings::PER_AVD_SETTINGS_NAME);
        QSettings avdSpecificSettings(avdSettingsFile, QSettings::IniFormat);

        const QMap<QString, QVariant> savedPosters =
                avdSpecificSettings
                        .value(Ui::Settings::PER_AVD_VIRTUAL_SCENE_POSTERS)
                        .toMap();
        const QMap<QString, QVariant> savedPosterSizes =
                avdSpecificSettings
                        .value(Ui::Settings::PER_AVD_VIRTUAL_SCENE_POSTER_SIZES)
                        .toMap();

        // Send the saved posters to the virtual scene.  This is called early
        // during emulator startup.  If a poster has been set via a command line
        // option, that poster will take priority and setInitialSetting will not
        // replace it.
        QMapIterator<QString, QVariant> it(savedPosters);
        while (it.hasNext()) {
            it.next();

            const QString& name = it.key();
            const QString path = it.value().toString();
            sVirtualSceneAgent->setInitialPoster(name.toUtf8().constData(),
                                                 path.toUtf8().constData());
            if (savedPosterSizes.contains(name)) {
                bool valid = false;
                const float size = savedPosterSizes[name].toFloat(&valid);
                if (valid) {
                    sVirtualSceneAgent->setPosterSize(name.toUtf8().constData(),
                                                      size);
                }
            }
        }
    }
}
