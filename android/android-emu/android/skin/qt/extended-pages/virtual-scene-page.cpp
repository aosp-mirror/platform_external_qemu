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
#include "android/metrics/MetricsReporter.h"
#include "android/metrics/proto/studio_stats.pb.h"

#include <QDebug>

VirtualScenePage::VirtualScenePage(QWidget* parent)
    : QWidget(parent), mUi(new Ui::VirtualScenePage()) {
    mUi->setupUi(this);

    connect(mUi->imageWall, SIGNAL(interaction()), this,
            SLOT(reportInteraction()));
    connect(mUi->imageTable, SIGNAL(interaction()), this,
            SLOT(reportInteraction()));
}

void VirtualScenePage::setVirtualSceneAgent(
        const QAndroidVirtualSceneAgent* agent) {
    mVirtualSceneAgent = agent;

    if (mHasBeenShown) {
        updatePosters();
    }
}

void VirtualScenePage::showEvent(QShowEvent* event) {
    if (!mHasBeenShown) {
        mHasBeenShown = true;
        updatePosters();
    }
}

void VirtualScenePage::on_imageWall_pathChanged(QString path) {
    if (mVirtualSceneAgent) {
        mVirtualSceneAgent->loadPoster("wall", path.toUtf8().constData());
    }
}

void VirtualScenePage::on_imageTable_pathChanged(QString path) {
    if (mVirtualSceneAgent) {
        mVirtualSceneAgent->loadPoster("table", path.toUtf8().constData());
    }
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

void VirtualScenePage::updatePosters() {
    if (!mVirtualSceneAgent) {
        return;
    }

    mVirtualSceneAgent->enumeratePosters(
            this,
            [](void* context, const char* posterName, const char* filename) {
                auto self = reinterpret_cast<VirtualScenePage*>(context);
                QString name(posterName);

                if (name == "wall") {
                    self->mUi->imageWall->setPath(filename);
                } else if (name == "table") {
                    self->mUi->imageTable->setPath(filename);
                } else {
                    qWarning() << "Unknown poster name " << name;
                }
            });
}
